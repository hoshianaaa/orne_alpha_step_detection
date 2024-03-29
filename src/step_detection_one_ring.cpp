#include <ros/ros.h> 
#include "sensor_msgs/PointCloud2.h" 
#include "sensor_msgs/PointCloud.h" 
#include "sensor_msgs/point_cloud_conversion.h" 
#include <pcl_conversions/pcl_conversions.h> 
#include <iostream>
#include <cmath>
#include <vector>
#define RING_NUMBER 0
#define RING_RAD 1.8
#define alpha 0.9
#define beta 1.1

class get_one_ring
{
public:
    get_one_ring();
private:
    ros::NodeHandle nh_;
    ros::Subscriber sub_;
    ros::Publisher pub_;
    ros::Publisher angular_pub_;
    ros::Publisher filtered_pub_;
    void pointsCallback(const sensor_msgs::PointCloud2ConstPtr& msg);
};

get_one_ring::get_one_ring(){
    sub_ = nh_.subscribe("velodyne_points", 1000, &get_one_ring::pointsCallback,this);
    pub_ = nh_.advertise<sensor_msgs::PointCloud2>("one_ring", 2, true);
    angular_pub_ = nh_.advertise<sensor_msgs::PointCloud2>("angular_ring", 2, true);
    filtered_pub_ = nh_.advertise<sensor_msgs::PointCloud>("filtered_cloud", 2, true);
}

void get_one_ring::pointsCallback(const sensor_msgs::PointCloud2ConstPtr& msg){



    //one_ring
    pcl::PointCloud<pcl::PointXYZ> cloud_pcl;
    cloud_pcl.width = msg->height * msg->width / 8;
    cloud_pcl.height = 1;
    cloud_pcl.points.resize(cloud_pcl.width * cloud_pcl.height);
    int ring_size = 1;

    for (uint j=0; j < msg->height * msg->width; j+=5){
        int arrayPosX = j * msg->point_step + msg->fields[0].offset;
        int arrayPosY = j * msg->point_step + msg->fields[1].offset;
        int arrayPosZ = j * msg->point_step + msg->fields[2].offset;
        int ring = msg->data[j * msg->point_step + msg->fields[4].offset];

        //ring number 0-15
        if(ring == RING_NUMBER){
            float x;
            float y;
            float z;

            memcpy(&x, &msg->data[arrayPosX], sizeof(float));
            memcpy(&y, &msg->data[arrayPosY], sizeof(float));
            memcpy(&z, &msg->data[arrayPosZ], sizeof(float)); 

            cloud_pcl.points[ring_size].x = x;
            cloud_pcl.points[ring_size].y = y;
            cloud_pcl.points[ring_size].z = z;
            ring_size++;
        }
    }

    sensor_msgs::PointCloud2 ring;
    pcl::toROSMsg(cloud_pcl, ring);
    ring.header.frame_id = "velodyne";
    pub_.publish(ring);



    //angular_ring
    std::vector<int> angular_cut_list;
    float cut_angular_r = 3.14/2.5;
    float cut_angular_l = 3.14/2;

    for(int i=0;i<ring_size;i++){
        float angular;

       angular = std::atan2(cloud_pcl.points[i].y, cloud_pcl.points[i].x);
        if((angular > -cut_angular_l) && (angular < cut_angular_r)){
            angular_cut_list.push_back(i);

            /////DEBUG (angular and distance)//////
            /*
            float debug_d;
            debug_d = sqrt((cloud_pcl.points[i].x * cloud_pcl.points[i].x) + (cloud_pcl.points[i].y * cloud_pcl.points[i].y));
            std::cout << angular << " " << debug_d << std::endl;
            */
            ////////////////
     
        }
    }

    pcl::PointCloud<pcl::PointXYZ> angular_pcl;
    angular_pcl.width = angular_cut_list.size();
    angular_pcl.height = 1;
    angular_pcl.points.resize(angular_cut_list.size());
    for(int i=0;i<angular_cut_list.size();i++)
    {
        int n = angular_cut_list.at(i);
        angular_pcl.points[i].x = cloud_pcl.points[n].x;
        angular_pcl.points[i].y = cloud_pcl.points[n].y;
        angular_pcl.points[i].z = cloud_pcl.points[n].z;
    }

    sensor_msgs::PointCloud2 angular_ring;
    pcl::toROSMsg(angular_pcl, angular_ring);
    angular_ring.header.frame_id = "velodyne";
    angular_pub_.publish(angular_ring);



    //filtered_ring
    int ang_size = angular_cut_list.size();
    float d = 0;
    std::vector<int> step_list;

    for(int i=0;i<ang_size;i++){
        d = sqrt((angular_pcl.points[i].x * angular_pcl.points[i].x) + (angular_pcl.points[i].y * angular_pcl.points[i].y));
        if(d < alpha * RING_RAD || d > beta * RING_RAD){
            step_list.push_back(i);
        }
    }

    pcl::PointCloud<pcl::PointXYZ> filtered_pcl;
    filtered_pcl.width = step_list.size();
    filtered_pcl.height = 1;
    filtered_pcl.points.resize(filtered_pcl.width * filtered_pcl.height);

    for(int i=0;i<step_list.size();i++){
        filtered_pcl.points[i].x = angular_pcl.points[step_list.at(i)].x;
        filtered_pcl.points[i].y = angular_pcl.points[step_list.at(i)].y;
        filtered_pcl.points[i].z = angular_pcl.points[step_list.at(i)].z;
    }

    sensor_msgs::PointCloud2 filtered_ring_2;
    sensor_msgs::PointCloud filtered_ring;

    pcl::toROSMsg(filtered_pcl, filtered_ring_2);
    filtered_ring_2.header.frame_id = "velodyne";

    sensor_msgs::convertPointCloud2ToPointCloud(filtered_ring_2, filtered_ring);

    filtered_pub_.publish(filtered_ring);
}

    
int main(int argc, char** argv)
{
    ros::init(argc, argv, "step_detection_one_ring");
    get_one_ring x;
    ros::spin();
    return 0;
}
