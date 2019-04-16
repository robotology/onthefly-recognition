// This is an automatically generated file.
// Generated from this std_msgs_Int32MultiArray.msg definition:
//   # Please look at the MultiArrayLayout message definition for
//   # documentation on all multiarrays.
//   
//   MultiArrayLayout  layout        # specification of data layout
//   int32[]           data          # array of data
//   
//   
// Instances of this class can be read and written with YARP ports,
// using a ROS-compatible format.

#ifndef YARPMSG_TYPE_std_msgs_Int32MultiArray
#define YARPMSG_TYPE_std_msgs_Int32MultiArray

#include <string>
#include <vector>
#include <yarp/os/Wire.h>
#include <yarp/os/idl/WireTypes.h>
#include "std_msgs_MultiArrayDimension.h"
#include "std_msgs_MultiArrayLayout.h"

class std_msgs_Int32MultiArray : public yarp::os::idl::WirePortable {
public:
  std_msgs_MultiArrayLayout layout;
  std::vector<yarp::os::NetInt32> data;

  std_msgs_Int32MultiArray() {
  }

  void clear() {
    // *** layout ***
    layout.clear();

    // *** data ***
    data.clear();
  }

  bool readBare(yarp::os::ConnectionReader& connection) YARP_OVERRIDE {
    // *** layout ***
    if (!layout.read(connection)) return false;

    // *** data ***
    int len = connection.expectInt();
    data.resize(len);
    if (len > 0 && !connection.expectBlock((char*)&data[0],sizeof(yarp::os::NetInt32)*len)) return false;
    return !connection.isError();
  }

  bool readBottle(yarp::os::ConnectionReader& connection) YARP_OVERRIDE {
    connection.convertTextMode();
    yarp::os::idl::WireReader reader(connection);
    if (!reader.readListHeader(2)) return false;

    // *** layout ***
    if (!layout.read(connection)) return false;

    // *** data ***
    if (connection.expectInt()!=(BOTTLE_TAG_LIST|BOTTLE_TAG_INT)) return false;
    int len = connection.expectInt();
    data.resize(len);
    for (int i=0; i<len; i++) {
      data[i] = (yarp::os::NetInt32)connection.expectInt();
    }
    return !connection.isError();
  }

  using yarp::os::idl::WirePortable::read;
  bool read(yarp::os::ConnectionReader& connection) YARP_OVERRIDE {
    if (connection.isBareMode()) return readBare(connection);
    return readBottle(connection);
  }

  bool writeBare(yarp::os::ConnectionWriter& connection) YARP_OVERRIDE {
    // *** layout ***
    if (!layout.write(connection)) return false;

    // *** data ***
    connection.appendInt(data.size());
    if (data.size()>0) {connection.appendExternalBlock((char*)&data[0],sizeof(yarp::os::NetInt32)*data.size());}
    return !connection.isError();
  }

  bool writeBottle(yarp::os::ConnectionWriter& connection) YARP_OVERRIDE {
    connection.appendInt(BOTTLE_TAG_LIST);
    connection.appendInt(2);

    // *** layout ***
    if (!layout.write(connection)) return false;

    // *** data ***
    connection.appendInt(BOTTLE_TAG_LIST|BOTTLE_TAG_INT);
    connection.appendInt(data.size());
    for (size_t i=0; i<data.size(); i++) {
      connection.appendInt((int)data[i]);
    }
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
  typedef yarp::os::idl::BareStyle<std_msgs_Int32MultiArray> rosStyle;
  typedef yarp::os::idl::BottleStyle<std_msgs_Int32MultiArray> bottleStyle;

  // Give source text for class, ROS will need this
  yarp::os::ConstString getTypeText() {
    return "# Please look at the MultiArrayLayout message definition for\n\
# documentation on all multiarrays.\n\
\n\
MultiArrayLayout  layout        # specification of data layout\n\
int32[]           data          # array of data\n\
\n\
\n================================================================================\n\
MSG: std_msgs/MultiArrayLayout\n\
# The multiarray declares a generic multi-dimensional array of a\n\
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
    yarp::os::Type typ = yarp::os::Type::byName("std_msgs/Int32MultiArray","std_msgs/Int32MultiArray");
    typ.addProperty("md5sum",yarp::os::Value("1d99f79f8b325b44fee908053e9c945b"));
    typ.addProperty("message_definition",yarp::os::Value(getTypeText()));
    return typ;
  }
};

#endif
