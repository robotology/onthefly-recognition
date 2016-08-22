/*
 * definitions.h
 *
 *  Created on: Aug 21, 2016
 *      Author: icub
 */

#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

#define                 ACK                 VOCAB3('a','c','k')
#define                 NACK                VOCAB4('n','a','c','k')

#define                 STATE_TRAINING      0
#define                 STATE_CLASSIFYING   1
#define					STATE_WHATISTHIS	2

#define                 MODE_ROBOT          0
#define                 MODE_HUMAN          1
#define                 CMD_ROBOT           VOCAB4('r','o','b','o')
#define                 CMD_HUMAN           VOCAB4('h','u','m','a')

#define					CROP_MODE_RADIUS	0
#define					CROP_MODE_BBDISP	1
#define					CMD_RADIUS			VOCAB4('r','a','d','i')
#define					CMD_BBDISP			VOCAB4('b','b','d','i')

#define					MAX_BUFFER_SIZE		900

#define                 CMD_TRAIN           VOCAB4('t','r','a','i')
#define                 CMD_FORGET          VOCAB4('f','o','r','g')
#define                 CMD_WHATISTHIS      VOCAB4('w','h','a','t')

#define                 CMD_SET         VOCAB3('s','e','t')
#define                 CMD_GET         VOCAB3('g','e','t')

#endif /* DEFINITIONS_H_ */
