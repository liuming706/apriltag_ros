// ros
#include "pose_estimation.hpp"
#include <apriltag_msgs/msg/april_tag_detection.hpp>
#include <apriltag_msgs/msg/april_tag_detection_array.hpp>
#ifdef cv_bridge_HPP
#include <cv_bridge/cv_bridge.hpp>
#else
#include <cv_bridge/cv_bridge.h>
#endif
#include <image_transport/camera_subscriber.hpp>
#include <image_transport/image_transport.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_components/register_node_macro.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <tf2_ros/transform_broadcaster.h>

// apriltag
#include "tag_functions.hpp"
#include <apriltag.h>
#include <Eigen/Dense>

#define IF(N, V) \
    if (assign_check(parameter, N, V)) continue;

template <typename T>
void assign(const rclcpp::Parameter &parameter, T &var)
{
    var = parameter.get_value<T>();
}

template <typename T>
void assign(const rclcpp::Parameter &parameter, std::atomic<T> &var)
{
    var = parameter.get_value<T>();
}

template <typename T>
bool assign_check(const rclcpp::Parameter &parameter, const std::string &name, T &var)
{
    if (parameter.get_name() == name) {
        assign(parameter, var);
        return true;
    }
    return false;
}

typedef Eigen::Matrix<double, 3, 3, Eigen::RowMajor> Mat3;

rcl_interfaces::msg::ParameterDescriptor descr(const std::string &description, const bool &read_only = false)
{
    rcl_interfaces::msg::ParameterDescriptor descr;

    descr.description = description;
    descr.read_only = read_only;

    return descr;
}

void getPose(const matd_t &H, const Mat3 &Pinv, geometry_msgs::msg::Transform &t, const double size)
{
    // compute extrinsic camera parameter
    // https://dsp.stackexchange.com/a/2737/31703
    // H = K * T  =>  T = K^(-1) * H
    const Mat3 T = Pinv * Eigen::Map<const Mat3>(H.data);
    Mat3 R;
    R.col(0) = T.col(0).normalized();
    R.col(1) = T.col(1).normalized();
    R.col(2) = R.col(0).cross(R.col(1));

    // rotate by half rotation about x-axis to have z-axis
    // point upwards orthogonal to the tag plane
    R.col(1) *= -1;
    R.col(2) *= -1;

    // the corner coordinates of the tag in the canonical frame are (+/-1, +/-1)
    // hence the scale is half of the edge size
    const Eigen::Vector3d tt = T.rightCols<1>() / ((T.col(0).norm() + T.col(0).norm()) / 2.0) * (size / 2.0);

    const Eigen::Quaterniond q(R);

    t.translation.x = tt.x();
    t.translation.y = tt.y();
    t.translation.z = tt.z();
    t.rotation.w = q.w();
    t.rotation.x = q.x();
    t.rotation.y = q.y();
    t.rotation.z = q.z();
}

class AprilTagNode : public rclcpp::Node
{
public:
    AprilTagNode(const rclcpp::NodeOptions &options);

    ~AprilTagNode() override;

private:
    void onCamera(const sensor_msgs::msg::Image::ConstSharedPtr &msg_img,
                  const sensor_msgs::msg::CameraInfo::ConstSharedPtr &msg_ci);

    rcl_interfaces::msg::SetParametersResult onParameter(const std::vector<rclcpp::Parameter> &parameters);

    void imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr &msg_img);
    void cameraInfoCallback(const sensor_msgs::msg::CameraInfo::ConstSharedPtr &msg_ci);
    const OnSetParametersCallbackHandle::SharedPtr cb_parameter;

private:
    apriltag_family_t *tf;
    apriltag_detector_t *const td;

    // parameter
    std::mutex mutex;
    double tag_edge_size;
    std::atomic<int> max_hamming;
    std::atomic<bool> profile;
    std::unordered_map<int, std::string> tag_frames;
    std::unordered_map<int, double> tag_sizes;

    std::function<void(apriltag_family_t *)> tf_destructor;

    // const image_transport::CameraSubscriber sub_cam;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_img_;
    rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr sub_camera_info_;

    const rclcpp::Publisher<apriltag_msgs::msg::AprilTagDetectionArray>::SharedPtr pub_detections;
    tf2_ros::TransformBroadcaster tf_broadcaster;
    std::unique_ptr<std::array<double, 4>> intrinsics_{nullptr};
    pose_estimation_f estimate_pose = nullptr;
};

RCLCPP_COMPONENTS_REGISTER_NODE(AprilTagNode)

AprilTagNode::AprilTagNode(const rclcpp::NodeOptions &options) :
    Node("apriltag", options),
    // parameter
    cb_parameter(add_on_set_parameters_callback(std::bind(&AprilTagNode::onParameter, this, std::placeholders::_1))),
    td(apriltag_detector_create()),
    // topics
    // sub_cam(image_transport::create_camera_subscription(
    //     this, this->get_node_topics_interface()->resolve_topic_name("image_rect"),
    //     std::bind(&AprilTagNode::onCamera, this, std::placeholders::_1, std::placeholders::_2),
    //     declare_parameter("image_transport", "raw", descr({}, true)), rmw_qos_profile_sensor_data)),
    pub_detections(create_publisher<apriltag_msgs::msg::AprilTagDetectionArray>("detections", rclcpp::QoS(1))),
    tf_broadcaster(this)
{
    // read-only parameters
    const std::string tag_family = declare_parameter("family", "36h11", descr("tag family", true));
    tag_edge_size = declare_parameter("size", 1.0, descr("default tag size", true));

    // get tag names, IDs and sizes
    const auto ids = declare_parameter("tag.ids", std::vector<int64_t>{}, descr("tag ids", true));
    const auto frames = declare_parameter("tag.frames", std::vector<std::string>{}, descr("tag frame names per id", true));
    const auto sizes = declare_parameter("tag.sizes", std::vector<double>{}, descr("tag sizes per id", true));

    // get method for estimating tag pose
    estimate_pose = pose_estimation_methods.at(
        declare_parameter("pose_estimation_method", "pnp",
                          descr("pose estimation method: \"pnp\" (more accurate) or \"homography\" (faster)", true)));

    // detector parameters in "detector" namespace
    declare_parameter("detector.threads", td->nthreads, descr("number of threads"));
    declare_parameter("detector.decimate", td->quad_decimate, descr("decimate resolution for quad detection"));
    declare_parameter("detector.blur", td->quad_sigma, descr("sigma of Gaussian blur for quad detection"));
    declare_parameter("detector.refine", td->refine_edges, descr("snap to strong gradients"));
    declare_parameter("detector.sharpening", td->decode_sharpening, descr("sharpening of decoded images"));
    declare_parameter("detector.debug", td->debug, descr("write additional debugging images to working directory"));

    declare_parameter("max_hamming", 0, descr("reject detections with more corrected bits than allowed"));
    declare_parameter("profile", false, descr("print profiling information to stdout"));

    if (!frames.empty()) {
        if (ids.size() != frames.size()) {
            throw std::runtime_error("Number of tag ids (" + std::to_string(ids.size()) + ") and frames (" +
                                     std::to_string(frames.size()) + ") mismatch!");
        }
        for (size_t i = 0; i < ids.size(); i++) {
            tag_frames[ids[i]] = frames[i];
        }
    }

    if (!sizes.empty()) {
        // use tag specific size
        if (ids.size() != sizes.size()) {
            throw std::runtime_error("Number of tag ids (" + std::to_string(ids.size()) + ") and sizes (" +
                                     std::to_string(sizes.size()) + ") mismatch!");
        }
        for (size_t i = 0; i < ids.size(); i++) {
            tag_sizes[ids[i]] = sizes[i];
        }
    }

    if (tag_fun.count(tag_family)) {
        tf = tag_fun.at(tag_family).first();
        tf_destructor = tag_fun.at(tag_family).second;
        apriltag_detector_add_family(td, tf);
    } else {
        throw std::runtime_error("Unsupported tag family: " + tag_family);
    }
    sub_camera_info_ = this->create_subscription<sensor_msgs::msg::CameraInfo>(
        this->get_node_topics_interface()->resolve_topic_name("camera_info"), 10,
        std::bind(&AprilTagNode::cameraInfoCallback, this, std::placeholders::_1));
    sub_img_ = this->create_subscription<sensor_msgs::msg::Image>(
        this->get_node_topics_interface()->resolve_topic_name("image"), 10,
        std::bind(&AprilTagNode::imageCallback, this, std::placeholders::_1));
}

AprilTagNode::~AprilTagNode()
{
    apriltag_detector_destroy(td);
    tf_destructor(tf);
}

void AprilTagNode::onCamera(const sensor_msgs::msg::Image::ConstSharedPtr &msg_img,
                            const sensor_msgs::msg::CameraInfo::ConstSharedPtr &msg_ci)

{
    // camera intrinsics for rectified images
    const std::array<double, 4> intrinsics = {msg_ci->p.data()[0], msg_ci->p.data()[5], msg_ci->p.data()[2],
                                              msg_ci->p.data()[6]};

    // precompute inverse projection matrix
    // const Mat3 Pinv =
    //     Eigen::Map<const Eigen::Matrix<double, 3, 4, Eigen::RowMajor> >(msg_ci->p.data()).leftCols<3>().inverse();

    // convert to 8bit monochrome image
    const cv::Mat img_uint8 = cv_bridge::toCvShare(msg_img, "mono8")->image;

    image_u8_t im{img_uint8.cols, img_uint8.rows, img_uint8.cols, img_uint8.data};

    // detect tags
    mutex.lock();
    zarray_t *detections = apriltag_detector_detect(td, &im);
    mutex.unlock();

    if (profile) timeprofile_display(td->tp);

    apriltag_msgs::msg::AprilTagDetectionArray msg_detections;
    msg_detections.header = msg_img->header;

    std::vector<geometry_msgs::msg::TransformStamped> tfs;
    std::cout << "detect " << zarray_size(detections) << " tags !" << std::endl;
    for (int i = 0; i < zarray_size(detections); i++) {
        apriltag_detection_t *det;
        zarray_get(detections, i, &det);

        RCLCPP_DEBUG(get_logger(), "detection %3d: id (%2dx%2d)-%-4d, hamming %d, margin %8.3f\n", i,
                     det->family->nbits, det->family->h, det->id, det->hamming, det->decision_margin);

        // ignore untracked tags
        if (!tag_frames.empty() && !tag_frames.count(det->id)) {
            continue;
        }

        // reject detections with more corrected bits than allowed
        if (det->hamming > max_hamming) {
            continue;
        }

        // detection
        apriltag_msgs::msg::AprilTagDetection msg_detection;
        msg_detection.family = std::string(det->family->name);
        msg_detection.id = det->id;
        msg_detection.hamming = det->hamming;
        msg_detection.decision_margin = det->decision_margin;
        msg_detection.centre.x = det->c[0];
        msg_detection.centre.y = det->c[1];
        std::memcpy(msg_detection.corners.data(), det->p, sizeof(double) * 8);
        std::memcpy(msg_detection.homography.data(), det->H->data, sizeof(double) * 9);
        msg_detections.detections.push_back(msg_detection);

        // 3D orientation and position
        geometry_msgs::msg::TransformStamped tf;
        tf.header = msg_img->header;
        // set child frame name by generic tag name or configured tag name
        tf.child_frame_id = tag_frames.count(det->id) ? tag_frames.at(det->id)
                                                      : std::string(det->family->name) + ":" + std::to_string(det->id);
        const double size = tag_sizes.count(det->id) ? tag_sizes.at(det->id) : tag_edge_size;
        if (estimate_pose != nullptr) {
            // 法1
            tf.transform = estimate_pose(det, intrinsics, size);
        }
        // 法2
        // getPose(*(det->H), Pinv, tf.transform, tag_sizes.count(det->id) ? tag_sizes.at(det->id) : tag_edge_size);

        tfs.push_back(tf);
    }

    pub_detections->publish(msg_detections);
    tf_broadcaster.sendTransform(tfs);

    apriltag_detections_destroy(detections);
}

void AprilTagNode::cameraInfoCallback(const sensor_msgs::msg::CameraInfo::ConstSharedPtr &msg_ci)

{
    if (intrinsics_) {
        return;
    }

    intrinsics_ = std::make_unique<std::array<double, 4>>(
        std::array<double, 4>{msg_ci->p.data()[0], msg_ci->p.data()[5], msg_ci->p.data()[2], msg_ci->p.data()[6]});
    std::cout << "intrinsics: " << (*intrinsics_)[0] << "," << (*intrinsics_)[1] << "," << (*intrinsics_)[2] << ","
              << (*intrinsics_)[3] << std::endl;
}

void AprilTagNode::imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr &msg_img)

{
    if (!intrinsics_) {
        return;
    }
    // // 法2: precompute inverse projection matrix
    // const Mat3 Pinv =
    //     Eigen::Map<const Eigen::Matrix<double, 3, 4, Eigen::RowMajor>>(msg_ci->p.data()).leftCols<3>().inverse();

    const cv::Mat img_uint8 = cv_bridge::toCvShare(msg_img, "mono8")->image;
    image_u8_t im{img_uint8.cols, img_uint8.rows, img_uint8.cols, img_uint8.data};

    // detect tags
    mutex.lock();
    zarray_t *detections = apriltag_detector_detect(td, &im);
    mutex.unlock();

    if (profile) timeprofile_display(td->tp);

    apriltag_msgs::msg::AprilTagDetectionArray msg_detections;
    msg_detections.header = msg_img->header;

    std::vector<geometry_msgs::msg::TransformStamped> tfs;
    std::cout << "lumen " << "detect " << zarray_size(detections) << " tags !" << std::endl;
    for (int i = 0; i < zarray_size(detections); i++) {
        apriltag_detection_t *det;
        zarray_get(detections, i, &det);

        RCLCPP_DEBUG(get_logger(), "detection %3d: id (%2dx%2d)-%-4d, hamming %d, margin %8.3f\n", i,
                     det->family->nbits, det->family->h, det->id, det->hamming, det->decision_margin);

        // ignore untracked tags
        if (!tag_frames.empty() && !tag_frames.count(det->id)) {
            continue;
        }

        // reject detections with more corrected bits than allowed
        if (det->hamming > max_hamming) {
            continue;
        }

        // detection
        apriltag_msgs::msg::AprilTagDetection msg_detection;
        msg_detection.family = std::string(det->family->name);
        msg_detection.id = det->id;
        msg_detection.hamming = det->hamming;
        msg_detection.decision_margin = det->decision_margin;
        msg_detection.centre.x = det->c[0];
        msg_detection.centre.y = det->c[1];
        std::memcpy(msg_detection.corners.data(), det->p, sizeof(double) * 8);
        std::memcpy(msg_detection.homography.data(), det->H->data, sizeof(double) * 9);
        msg_detections.detections.push_back(msg_detection);

        // 3D orientation and position
        geometry_msgs::msg::TransformStamped tf;
        tf.header = msg_img->header;
        // set child frame name by generic tag name or configured tag name
        tf.child_frame_id = tag_frames.count(det->id) ? tag_frames.at(det->id)
                                                      : std::string(det->family->name) + ":" + std::to_string(det->id);
        const double size = tag_sizes.count(det->id) ? tag_sizes.at(det->id) : tag_edge_size;
        // 法1
        if (estimate_pose != nullptr) {
            tf.transform = estimate_pose(det, *intrinsics_, size);
        }
        // // 法2
        // getPose(*(det->H), Pinv, tf.transform, tag_sizes.count(det->id) ? tag_sizes.at(det->id) : tag_edge_size);

        tfs.push_back(tf);
    }

    pub_detections->publish(msg_detections);
    tf_broadcaster.sendTransform(tfs);

    apriltag_detections_destroy(detections);
}

rcl_interfaces::msg::SetParametersResult AprilTagNode::onParameter(const std::vector<rclcpp::Parameter> &parameters)
{
    rcl_interfaces::msg::SetParametersResult result;

    mutex.lock();

    for (const rclcpp::Parameter &parameter : parameters) {
        RCLCPP_DEBUG_STREAM(get_logger(), "setting: " << parameter);

        IF("detector.threads", td->nthreads)
        IF("detector.decimate", td->quad_decimate)
        IF("detector.blur", td->quad_sigma)
        IF("detector.refine", td->refine_edges)
        IF("detector.sharpening", td->decode_sharpening)
        IF("detector.debug", td->debug)
        IF("max_hamming", max_hamming)
        IF("profile", profile)
    }

    mutex.unlock();

    result.successful = true;

    return result;
}
