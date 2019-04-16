// ROS headers
#include <ros/ros.h>
#include <pcl_ros/point_cloud.h>
#include <std_msgs/Int32MultiArray.h>
#include <tf/transform_listener.h>

// PCL specific includes
#include <sensor_msgs/PointCloud2.h>
#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/PoseStamped.h>

#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

typedef pcl::PointCloud<pcl::PointXYZ> PointCloud;

int u = -1;
int v = -1;

ros::Publisher pub_coords3d;
tf::StampedTransform transform;
tf::TransformListener* tran;

void point2pose(const geometry_msgs::PointStamped& coords3D, geometry_msgs::PoseStamped& gaze)
{
    
    gaze.header.frame_id = coords3D.header.frame_id;
    gaze.pose.position.x = coords3D.point.x;  // coords are swapped in image and robot camera frame
    gaze.pose.position.y = coords3D.point.y;
    gaze.pose.position.z = coords3D.point.z;
    gaze.pose.orientation.w = 0;
    gaze.pose.orientation.x = 0.0;
    gaze.pose.orientation.y = 0.0;
    gaze.pose.orientation.z = 0.0;
    
    return;
}

void pixelTo3DPoint_PCL(const PointCloud::ConstPtr& cloud, const int u, const int v, geometry_msgs::PointStamped &coords3D)
    {
      // get width and height of 2D point cloud data
      int width = cloud->width;
      int height = cloud->height;

     // Convert from u (column / width), v (row/height) to position in array
      int i = (u) + (v)*cloud->width;
      ROS_INFO("Received cloud of size (%d, %d)", width, height);

      coords3D.header.frame_id = "multisense/head";
      coords3D.header.stamp = ros::Time::now();
      coords3D.point.x = cloud->points[i].x;
      coords3D.point.y = cloud->points[i].y;
      coords3D.point.z = cloud->points[i].z;

      return;
    }


void cloud_callback_PCL(const PointCloud::ConstPtr& input)
{
    // Check that 2D coords are already available
    if ((u<0)&& (v<0)) {
        ROS_INFO("Coords not available yet");
        return;
    }
    
    geometry_msgs::PointStamped point3D_cam;
    geometry_msgs::PointStamped point3D_waist;
    geometry_msgs::PoseStamped gaze_pose;

    // Use the pointcloud to transform 2D to 3D coordinates
    pixelTo3DPoint_PCL(input, u, v, point3D_cam);    
    ROS_INFO("Coords of (%d, %d) are (%f, %f, %f)",u,v, point3D_cam.point.x, point3D_cam.point.y, point3D_cam.point.z);
    
    // Transform 3D coordinates from the camera reference frame to the Waist reference frame.
    try{
    	tran->transformPoint("Waist", point3D_cam, point3D_waist);
    }
    	catch (tf::TransformException& ex) {
    	ROS_ERROR("%s",ex.what());
        ros::Duration(1.0).sleep();  
	}

    ROS_INFO("Coords on Waist RF are (%f, %f, %f)", point3D_waist.point.x, point3D_waist.point.y, point3D_waist.point.z);
    
    // Format the gaze command based on the Waist 3D points
    point2pose(point3D_waist, gaze_pose);
    
    // Send the 3D point as a gaze command.
    pub_coords3d.publish(gaze_pose);
    
    return;
}


void coords_callback(const std_msgs::Int32MultiArray::ConstPtr& coords)
{
    u = coords->data[0];
    v = coords->data[1];
    return;
}


int main(int argc, char** argv)
{
    ros::init(argc, argv, "track_blob");
    ros::NodeHandle nh;

    // Create Subscriber to Pointclouds from Multisense camera
    ros::Subscriber pointcloud_sub = nh.subscribe<PointCloud>("/multisense/organized_image_points2", 1, cloud_callback_PCL);

    // Create a ROS subscriber to 2D points from dispBlobber (formatted to ROS by OTFR_ROS)
    ros::Subscriber coords2D_sub = nh.subscribe("/OTFR_ROS/coords_out", 1, coords_callback);

    // Read transform from camera to Waist
    tf::TransformListener listener;
    tran = &listener;

    // Create a publisher to send out the 3D coordinates of point to gaze to
    pub_coords3d = nh.advertise<geometry_msgs::PoseStamped>("Waist_T_gaze", 1);

    ROS_INFO("Spinning");
    ros::spin();
}
