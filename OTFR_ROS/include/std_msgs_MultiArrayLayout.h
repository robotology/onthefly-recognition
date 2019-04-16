// This is an automatically generated file.
// Generated from this std_msgs_MultiArrayLayout.msg definition:
//   # The multiarray declares a generic multi-dimensional array of a
//   # particular data type.  Dimensions are ordered from outer most
//   # to inner most.
//   
//   MultiArrayDimension[] dim # Array of dimension properties
//   uint32 data_offset        # padding elements at front of data
//   
//   # Accessors should ALWAYS be written in terms of dimension stride
//   # and specified outer-most dimension first.
//   # 
//   # multiarray(i,j,k) = data[data_offset + dim_stride[1]*i + dim_stride[2]*j + k]
//   #
//   # A standard, 3-channel 640x480 image with interleaved color channels
//   # would be specified as:
//   #
//   # dim[0].label  = "height"
//   # dim[0].size   = 480
//   # dim[0].stride = 3*640*480 = 921600  (note dim[0] stride is just size of image)
//   # dim[1].label  = "width"
//   # dim[1].size   = 640
//   # dim[1].stride = 3*640 = 1920
//   # dim[2].label  = "channel"
//   # dim[2].size   = 3
//   # dim[2].stride = 3
//   #
//   # multiarray(i,j,k) refers to the ith row, jth column, and kth channel.
//   
// Instances of this class can be read and written with YARP ports,
// using a ROS-compatible format.

#ifndef YARPMSG_TYPE_std_msgs_MultiArrayLayout
#define YARPMSG_TYPE_std_msgs_MultiArrayLayout

#include <string>
#include <vector>
#include <yarp/os/Wire.h>
#include <yarp/os/idl/WireTypes.h>
#include "std_msgs_MultiArrayDimension.h"

class std_msgs_MultiArrayLayout : public yarp::os::idl::WirePortable {
public:
  std::vector<std_msgs_MultiArrayDimension> dim;
  yarp::os::NetUint32 data_offset;

  std_msgs_MultiArrayLayout() {
  }

  void clear() {
    // *** dim ***
    dim.clear();

    // *** data_offset ***
    data_offset = 0;
  }

  bool readBare(yarp::os::ConnectionReader& connection) YARP_OVERRIDE {
    // *** dim ***
    int len = connection.expectInt();
    dim.resize(len);
    for (int i=0; i<len; i++) {
      if (!dim[i].read(connection)) return false;
    }

    // *** data_offset ***
    data_offset = connection.expectInt();
    return !connection.isError();
  }

  bool readBottle(yarp::os::ConnectionReader& connection) YARP_OVERRIDE {
    connection.convertTextMode();
    yarp::os::idl::WireReader reader(connection);
    if (!reader.readListHeader(2)) return false;

    // *** dim ***
    if (connection.expectInt()!=BOTTLE_TAG_LIST) return false;
    int len = connection.expectInt();
    dim.resize(len);
    for (int i=0; i<len; i++) {
      if (!dim[i].read(connection)) return false;
    }

    // *** data_offset ***
    data_offset = reader.expectInt();
    return !connection.isError();
  }

  using yarp::os::idl::WirePortable::read;
  bool read(yarp::os::ConnectionReader& connection) YARP_OVERRIDE {
    if (connection.isBareMode()) return readBare(connection);
    return readBottle(connection);
  }

  bool writeBare(yarp::os::ConnectionWriter& connection) YARP_OVERRIDE {
    // *** dim ***
    connection.appendInt(dim.size());
    for (size_t i=0; i<dim.size(); i++) {
      if (!dim[i].write(connection)) return false;
    }

    // *** data_offset ***
    connection.appendInt(data_offset);
    return !connection.isError();
  }

  bool writeBottle(yarp::os::ConnectionWriter& connection) YARP_OVERRIDE {
    connection.appendInt(BOTTLE_TAG_LIST);
    connection.appendInt(2);

    // *** dim ***
    connection.appendInt(BOTTLE_TAG_LIST);
    connection.appendInt(dim.size());
    for (size_t i=0; i<dim.size(); i++) {
      if (!dim[i].write(connection)) return false;
    }

    // *** data_offset ***
    connection.appendInt(BOTTLE_TAG_INT);
    connection.appendInt((int)data_offset);
    connection.convertTextMode();
    return !connection.isError();
  }

  using yarp::os::idl::WirePortable::write;
  bool write(yarp::os::ConnectionWriter& connection) YARP_OVERRIDE {
    if (connection.isBareMode()) return writeBare(connection);
    return writeBottle(connection);
  }

  // This class will serialize ROS style or YARP style depending on protocol.
  // If you need to force a serialization style, use one of these classes:
  typedef yarp::os::idl::BareStyle<std_msgs_MultiArrayLayout> rosStyle;
  typedef yarp::os::idl::BottleStyle<std_msgs_MultiArrayLayout> bottleStyle;

  // Give source text for class, ROS will need this
  yarp::os::ConstString getTypeText() {
    return "# The multiarray declares a generic multi-dimensional array of a\n\
# particular data type.  Dimensions are ordered from outer most\n\
# to inner most.\n\
\n\
MultiArrayDimension[] dim # Array of dimension properties\n\
uint32 data_offset        # padding elements at front of data\n\
\n\
# Accessors should ALWAYS be written in terms of dimension stride\n\
# and specified outer-most dimension first.\n\
# \n\
# multiarray(i,j,k) = data[data_offset + dim_stride[1]*i + dim_stride[2]*j + k]\n\
#\n\
# A standard, 3-channel 640x480 image with interleaved color channels\n\
# would be specified as:\n\
#\n\
# dim[0].label  = \"height\"\n\
# dim[0].size   = 480\n\
# dim[0].stride = 3*640*480 = 921600  (note dim[0] stride is just size of image)\n\
# dim[1].label  = \"width\"\n\
# dim[1].size   = 640\n\
# dim[1].stride = 3*640 = 1920\n\
# dim[2].label  = \"channel\"\n\
# dim[2].size   = 3\n\
# dim[2].stride = 3\n\
#\n\
# multiarray(i,j,k) refers to the ith row, jth column, and kth channel.\n\
\n================================================================================\n\
MSG: std_msgs/MultiArrayDimension\n\
string label   # label of given dimension\n\
uint32 size    # size of given dimension (in type units)\n\
uint32 stride  # stride of given dimension";
  }

  // Name the class, ROS will need this
  yarp::os::Type getType() YARP_OVERRIDE {
    yarp::os::Type typ = yarp::os::Type::byName("std_msgs/MultiArrayLayout","std_msgs/MultiArrayLayout");
    typ.addProperty("md5sum",yarp::os::Value("0fed2a11c13e11c5571b4e2a995a91a3"));
    typ.addProperty("message_definition",yarp::os::Value(getTypeText()));
    return typ;
  }
};

#endif
