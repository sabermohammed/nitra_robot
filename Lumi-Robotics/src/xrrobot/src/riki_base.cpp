#include <ros/ros.h>
#include <nav_msgs/Odometry.h>
#include <tf/transform_broadcaster.h>
#include <riki_base.h>
#include "geometry_msgs/Vector3.h"
#include "geometry_msgs/Quaternion.h"

RikiBase::RikiBase():
    linear_velocity_x_(0),
    linear_velocity_y_(0),
    angular_velocity_z_(0),
    last_vel_time_(0),
    vel_dt_(0),
    x_pos_(0),
    y_pos_(0),
    heading_(0)
{
    ros::NodeHandle nh_private("~");
    odom_publisher_ = nh_.advertise<nav_msgs::Odometry>("raw_odom", 50);
    velocity_subscriber_ = nh_.subscribe("raw_vel", 50, &RikiBase::velCallback, this);
    //ros::Subscriber cartographer_subscriber_ = nh_.subscribe("tracked_pose", 50, &RikiBase::cartographerCallback, this);
    nh_private.getParam("linear_scale", linear_scale_);

    
    //ROS_INFO("linear_scale_: %f", linear_scale_);
}

void RikiBase::cartographerCallback(const geometry_msgs::PoseStamped& pose)
{
    ros::Time current_time = ros::Time::now();

    x_pos_ = pose.pose.position.x;
    y_pos_ = pose.pose.position.y;
    geometry_msgs::Quaternion odom_quat_msgs = pose.pose.orientation;
    
    // the incoming geometry_msgs::Quaternion is transformed to a tf::Quatenion
    tf::Quaternion odom_quat;
    tf::quaternionMsgToTF(odom_quat_msgs, odom_quat);
    
    // the tf::Quaternion has a method to access roll pitch and yaw
    double roll, pitch, yaw;
    tf::Matrix3x3(odom_quat).getRPY(roll, pitch, yaw);
    
    tf::Quaternion invert_odom_quat(roll, pitch, yaw + 3.1415);
    
    geometry_msgs::Quaternion invert_odom_quat_msgs;

    tf::quaternionTFToMsg(invert_odom_quat, invert_odom_quat_msgs);

    geometry_msgs::TransformStamped odom_trans;
    odom_trans.header.frame_id = "odom";
    odom_trans.child_frame_id = "base_footprint";
    //odom_trans.child_frame_id = "base_link"; //Trung test
    //robot's position in x,y, and z
    odom_trans.transform.translation.x = x_pos_;
    odom_trans.transform.translation.y = y_pos_;
    odom_trans.transform.translation.z = 0.0;
    //robot's heading in quaternion
    odom_trans.transform.rotation = invert_odom_quat_msgs;
    odom_trans.header.stamp = current_time;
    //publish robot's tf using odom_trans object
    //odom_broadcaster_.sendTransform(odom_trans);

    nav_msgs::Odometry odom;
    odom.header.stamp = current_time;
    odom.header.frame_id = "odom";
    odom.child_frame_id = "base_footprint";
    //odom.child_frame_id = "base_link"; //Trung test

    //robot's position in x,y, and z
    odom.pose.pose.position.x = x_pos_;
    odom.pose.pose.position.y = y_pos_;
    odom.pose.pose.position.z = 0.0;
    //robot's heading in quaternion
    odom.pose.pose.orientation = invert_odom_quat_msgs;
    
    odom.pose.covariance[0] = 0.001;
    odom.pose.covariance[7] = 0.001;
    odom.pose.covariance[35] = 0.001;

    //linear speed from encoders
    odom.twist.twist.linear.x = linear_velocity_x_;
    odom.twist.twist.linear.y = linear_velocity_y_;
    odom.twist.twist.linear.z = 0.0;

    odom.twist.twist.angular.x = 0.0;
    odom.twist.twist.angular.y = 0.0;
    //angular speed from encoders
    odom.twist.twist.angular.z = angular_velocity_z_;
    odom.twist.covariance[0] = 0.0001;
    odom.twist.covariance[7] = 0.0001;
    odom.twist.covariance[35] = 0.0001;

    odom_publisher_.publish(odom);
}

void RikiBase::velCallback(const riki_msgs::Velocities& vel)
{
    ros::Time current_time = ros::Time::now();

    linear_velocity_x_ = vel.linear_x * linear_scale_ ;
    linear_velocity_y_ = vel.linear_y * linear_scale_ ;
    angular_velocity_z_ = vel.angular_z;

    vel_dt_ = (current_time - last_vel_time_).toSec();
    last_vel_time_ = current_time;

    double delta_heading = angular_velocity_z_ * vel_dt_ ; //radians
    double delta_x = (linear_velocity_x_ * cos(heading_) - linear_velocity_y_ * sin(heading_)) * vel_dt_ ; //m
    double delta_y = (linear_velocity_x_ * sin(heading_) + linear_velocity_y_ * cos(heading_)) * vel_dt_ ; //m

    //calculate current position of the robot
    x_pos_ += delta_x;
    y_pos_ += delta_y;
    heading_ += delta_heading;

    //calculate robot's heading in quaternion angle
    //ROS has a function to calculate yaw in quaternion angle
    geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(heading_);

    geometry_msgs::TransformStamped odom_trans;
    odom_trans.header.frame_id = "odom";
    odom_trans.child_frame_id = "base_footprint";
    //odom_trans.child_frame_id = "base_link"; //Trung test
    //robot's position in x,y, and z
    odom_trans.transform.translation.x = x_pos_;
    odom_trans.transform.translation.y = y_pos_;
    odom_trans.transform.translation.z = 0.0;
    //robot's heading in quaternion
    odom_trans.transform.rotation = odom_quat;
    odom_trans.header.stamp = current_time;
    //publish robot's tf using odom_trans object
    //odom_broadcaster_.sendTransform(odom_trans);

    nav_msgs::Odometry odom;
    odom.header.stamp = current_time;
    odom.header.frame_id = "odom";
    odom.child_frame_id = "base_footprint";
    //odom.child_frame_id = "base_link"; //Trung test

    //robot's position in x,y, and z
    odom.pose.pose.position.x = x_pos_;
    odom.pose.pose.position.y = y_pos_;
    odom.pose.pose.position.z = 0.0;
    //robot's heading in quaternion
    odom.pose.pose.orientation = odom_quat;
    odom.pose.covariance[0] = 0.001;
    odom.pose.covariance[7] = 0.001;
    odom.pose.covariance[35] = 0.001;

    //linear speed from encoders
    odom.twist.twist.linear.x = linear_velocity_x_;
    odom.twist.twist.linear.y = linear_velocity_y_;
    odom.twist.twist.linear.z = 0.0;

    odom.twist.twist.angular.x = 0.0;
    odom.twist.twist.angular.y = 0.0;
    //angular speed from encoders
    odom.twist.twist.angular.z = angular_velocity_z_;
    odom.twist.covariance[0] = 0.0001;
    odom.twist.covariance[7] = 0.0001;
    odom.twist.covariance[35] = 0.0001;

    odom_publisher_.publish(odom);
    
}
