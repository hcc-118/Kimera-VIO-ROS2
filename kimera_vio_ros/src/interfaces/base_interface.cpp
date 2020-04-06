#include <chrono>

#include "kimera_vio_ros/interfaces/base_interface.hpp"

using namespace std::chrono_literals;

namespace kimera_vio_ros
{
namespace interfaces
{

BaseInterface::BaseInterface(
  rclcpp::Node::SharedPtr & node)
: VIO::DataProviderInterface(),
  node_(node),
  vio_params_(nullptr),
  vio_pipeline_(nullptr),
  tf_buffer_(nullptr),
  tf_broadcaster_(nullptr),
  tf_listener_(nullptr)
{
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(node_->get_clock());
  tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(node_);
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_, node_, false);

  callback_group_pipeline_ = node_->create_callback_group(
    rclcpp::callback_group::CallbackGroupType::MutuallyExclusive);

  base_link_frame_id_ = node_->declare_parameter(
    "frame_id.base_link", "base_link");
  map_frame_id_ = node_->declare_parameter(
    "frame_id.map", "map");
  world_frame_id_ = node_->declare_parameter(
    "frame_id.world", "world");

  std::string vio_params_folder_path_;
  vio_params_folder_path_ = node_->declare_parameter(
    "vio_params.folder_path", "");
  CHECK(!vio_params_folder_path_.empty());
  vio_params_ = std::make_shared<VIO::VioParams>(
    vio_params_folder_path_);

  vio_pipeline_.reset();
  vio_pipeline_ = VIO::make_unique<VIO::Pipeline>(*vio_params_);
}

BaseInterface::~BaseInterface()
{
  vio_pipeline_->shutdown();
  if (vio_params_->parallel_run_) {
    handle_pipeline_.get();
  }
}

void BaseInterface::start()
{
  if (vio_params_->parallel_run_) {
    handle_pipeline_ = std::async(
      std::launch::async,
      &VIO::Pipeline::spin,
      vio_pipeline_.get());
  } else {
    pipeline_timer_ = node_->create_wall_timer(
      10ms,
      std::bind(
        &VIO::Pipeline::spin,
        vio_pipeline_.get()),
      callback_group_pipeline_);
  }

}

}  // namespace interfaces
}  // namespace kimera_vio_ros
