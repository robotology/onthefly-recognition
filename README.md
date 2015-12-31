On The Fly Recognition
====================

This demo allows to teach the iCub to visually recognize new objects "on the fly".

Two modalities are available: `human` and `robot`, depending on whether the teacher shows the object to the iCub in his/her own hand or the robot itself holds and explore the object.

## Human modality

In this setting, the operator stands in front of the iCub, holding an object in the hand. When he/she issues the command to observe the new `object_name` (verbally or by typing), the robot starts focusing on the object of interest and keeps on tracking it for a fixed arbitrary period, so that the teacher can show the object to the robot, e.g., from different viewpoints. 

During this period, the recognition pipeline processes the incoming frames by cropping them around the object and extracting and storing a vectorial representation for each cropped frame. Once finished, the recognition system is trained on the collected examples, automatically labeled with the name provided by the human teacher.

This procedure can be repeated as many times as the number of objects that the robot has to learn.

At test time, the same interaction can be used to show objects and ask the robot to recognize them (simply issuing the command of recognizing an object instead of observing a given one). In this case, each frame is predicted independently from the others, or a buffer of arbitrary length can be used in order to average the predictions over a time window.

#### Tracking

In this modality, the robot can track the object either by using the motion cue or the disparity cue. 

###### Motion

If using motion (`onthefly-recognition_motion.xml`), then the robot detects the independent motion in the visual field and focuses on the biggest blob of motion. The [motionCUT](http://wiki.icub.org/brain/group__motionCUT.html) module is employed in this case. If the operator moves the object continuously during the tracking period, then this module allows to confidently detect the object in this setting.

###### Disparity

If using disparity (`onthefly-recognition_depth.xml`), then the robot focuses on the closest blob of points of similar depth (the biggest above a certain size threshold). The [dispBlobber](https://github.com/robotology/segmentation/tree/master/dispBlobber) module is employed in this case. With respect to the previous one, this tracking method allows also to keep the object almost still or to move it slowly, in a natural way. Indeed, in this setting the assumption holds that the object of interest is the closest to the robot in the visual field.

## Robot modality

In this setting, when when the teacher issues the command to observe the new `object_name` (or to recognize some object), the robot waits for an object to be put in its hand. Once done, it starts moving the hand to visually explore the object from different viewpoints and, once finished, it re-opens the hand so that the operator can take the object back.

The commands, the recognition pipeline and also the training and testing phases are structured in the same way as in the other modality.

#### Tracking

###### Proprioception 

In this modality, the robot focuses on the object simply because it knows the position of its hand.

## Recognition pipeline
 
The recognition pipeline that is used in this demo is extensively described and benchmarked in this paper: 

[Teaching iCub to recognize objects using deep Convolutional Neural Networks](http://jmlr.csail.mit.edu/proceedings/papers/v43/pasquale15.pdf) *Giulia Pasquale, Carlo Ciliberto, Francesca Odone, Lorenzo Rosasco and Lorenzo Natale*, 
Proceedings of The 4th Workshop on Machine Learning for Interactive Systems, pp. 21–25, 2015

    @inproceedings{pasquale15,
      author  = {Giulia Pasquale and Carlo Ciliberto and Francesca Odone and Lorenzo Rosasco and Lorenzo Natale},
      title   = {Teaching iCub to recognize objects using deep Convolutional Neural Networks},
      journal = {Proceedings of the 4th Workshop on Machine Learning for Interactive Systems, 32nd International Conference on Machine Learning},
      year    = {2015},
      url     = {http://jmlr.csail.mit.edu/proceedings/papers/v43/pasquale15.pdf}
      }

And it is based on the following steps:

1. **Segmentation** A rectangular region around the object of interest is cropped from the frame (either by using motion, disparity or proprioception - see the *Tracking* sections)
2. **Representation extraction** A vectorial representation is extracted from each crop. This and the following step are performed by employing the [Hierarchical Image Representation](https://github.com/robotology/himrep) repository. We are currently using off-the-shelf deep Conv Nets and in particular [Caffe](http://caffe.berkeleyvision.org/) Deep Learning framework (see the [caffeCoder](https://github.com/robotology/himrep) module for details). Nevertheless, it is still possible to employ a Sparse Coding encoding ([sparseCoder](https://github.com/robotology/himrep) module).
3. **Classification** Each extracted representation is fed to a linear classifier ([linearClassifierModule](https://github.com/robotology/himrep) module) that either uses it as a labeled example or predicts its class, depending whether we are in the training or in the testing phase. 

## Object identification or categorization?

Of course, the two tasks can be performed seamlessy, depending on the label that is assigned to the taught objects. 

## Installation

#### Dependencies

- [YARP](https://github.com/robotology/yarp)
- [iCub](https://github.com/robotology/icub-main)
- [icub-contrib-common](https://github.com/robotology/icub-contrib-common)
- [OpenCV](http://opencv.org/downloads.html)
- [LUA](http://www.lua.org/download.html)
- [Hierarchical Image Representation](https://github.com/robotology/himrep)
- [speechRecognizer](https://github.com/robotology/speech)

Ensure to have the required dependencies and then clone and compile the repository in the usual way with CMake.

## License

Material included here is Copyright of _iCub Facility - Istituto Italiano di Tecnologia_ and is released under the terms of the GPL v2.0 or later. See the file LICENSE for details.
