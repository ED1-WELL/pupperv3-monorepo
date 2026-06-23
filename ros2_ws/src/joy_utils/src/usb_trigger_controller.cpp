#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include <fcntl.h>
#include <string>
#include <termios.h>
#include <unistd.h>

class UsbTriggerController : public rclcpp::Node {
public:
  UsbTriggerController()
      : Node("usb_trigger_controller"), triggered_(false), serial_fd_(-1) {
    this->declare_parameter<int>("l2_axis_index", 2);
    this->declare_parameter<double>("l2_threshold", 0.0);
    this->declare_parameter<std::string>("serial_port", "/dev/ttyUSB0");
    this->declare_parameter<int>("baud_rate", 9600);

    this->get_parameter("l2_axis_index", l2_axis_index_);
    this->get_parameter("l2_threshold", l2_threshold_);
    this->get_parameter("serial_port", serial_port_);
    this->get_parameter("baud_rate", baud_rate_);

    open_serial();

    joy_sub_ = this->create_subscription<sensor_msgs::msg::Joy>(
        "/joy", 10,
        std::bind(&UsbTriggerController::joy_callback, this,
                  std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(),
                "UsbTriggerController started. L2 axis: %d, threshold: %.2f, "
                "serial: %s @ %d baud",
                l2_axis_index_, l2_threshold_, serial_port_.c_str(),
                baud_rate_);
  }

  ~UsbTriggerController() {
    if (serial_fd_ >= 0) {
      send_signal(false);
      close(serial_fd_);
    }
  }

private:
  void open_serial() {
    serial_fd_ = open(serial_port_.c_str(), O_WRONLY | O_NOCTTY | O_NONBLOCK);
    if (serial_fd_ < 0) {
      RCLCPP_WARN(this->get_logger(),
                  "Could not open serial port %s — trigger signals will not "
                  "be sent. Check the port name in config.yaml.",
                  serial_port_.c_str());
      return;
    }

    struct termios tty {};
    tcgetattr(serial_fd_, &tty);

    speed_t speed = B9600;
    if (baud_rate_ == 115200) speed = B115200;
    else if (baud_rate_ == 57600) speed = B57600;
    else if (baud_rate_ == 38400) speed = B38400;
    else if (baud_rate_ == 19200) speed = B19200;

    cfsetospeed(&tty, speed);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~HUPCL; // disable DTR drop on close — prevents Arduino reset
    tcsetattr(serial_fd_, TCSANOW, &tty);

    // Clear DTR now so the Arduino doesn't reset when we open the port
    int flags;
    ioctl(serial_fd_, TIOCMGET, &flags);
    flags &= ~TIOCM_DTR;
    ioctl(serial_fd_, TIOCMSET, &flags);

    RCLCPP_INFO(this->get_logger(), "Opened serial port %s", serial_port_.c_str());
  }

  void send_signal(bool on) {
    if (serial_fd_ < 0) return;
    const char byte = on ? '1' : '0';
    ssize_t written = write(serial_fd_, &byte, 1);
    if (written != 1) {
      RCLCPP_WARN(this->get_logger(),
                  "Failed to write to serial port %s", serial_port_.c_str());
    } else {
      RCLCPP_INFO(this->get_logger(), "Sent '%c' to Arduino", byte);
    }
  }

  void joy_callback(const sensor_msgs::msg::Joy::SharedPtr msg) {
    if (static_cast<int>(msg->axes.size()) <= l2_axis_index_) {
      return;
    }

    bool now_triggered = msg->axes[l2_axis_index_] > l2_threshold_;
    if (now_triggered != triggered_) {
      triggered_ = now_triggered;
      send_signal(triggered_);
    }
  }

  int l2_axis_index_;
  double l2_threshold_;
  std::string serial_port_;
  int baud_rate_;
  bool triggered_;
  int serial_fd_;

  rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
};

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<UsbTriggerController>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
