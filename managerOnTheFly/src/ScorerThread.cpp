/*
 * Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
 * Authors: Giulia Pasquale, Carlo Ciliberto
 * emails:  giulia.pasquale@iit.it, carlo.ciliberto@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */

#include <ScorerThread.h>
#include <definitions.h>

bool ScorerThread::threadInit()
{
	verbose = rf.check("verbose");

	string name = rf.find("name").asString().c_str();

	buffer_size = rf.check("buffer_size", Value(30)).asInt();
	confidence_width = rf.check("confidence_width",Value(800)).asInt();
	confidence_height = rf.check("confidence_height",Value(500)).asInt();

	//Ports
	//-----------------------------------------------------------
	//input
	port_in_scores.open(("/"+name+"/scores:i").c_str());

	//output
	port_out_confidence.open(("/"+name+"/confidence:o").c_str());
	//------------------------------------------------------------

	predicted_class = "?";

	histColorsCode.push_back(cv::Scalar( 65, 47,213));
	histColorsCode.push_back(cv::Scalar(122, 79, 58));
	histColorsCode.push_back(cv::Scalar(154,208, 72));
	histColorsCode.push_back(cv::Scalar( 71,196,249));
	histColorsCode.push_back(cv::Scalar(224,176, 96));
	histColorsCode.push_back(cv::Scalar( 22,118,238));

	return true;
}


void ScorerThread::run()
{

	mutex.wait();

	Bottle *bot=port_in_scores.read(false);

	if (bot==NULL)
	{
		if (scores_buffer.size()==0)
        {
            predicted_class = "?";

            vector<int> no_votes;
            draw_hist(no_votes);
        }

        mutex.post();
		return;
	}

	int n_classes = bot->size();
	if (n_classes==0)
	{
        std::cout << "Undefined reply from classifier: empty bottle of scores!" << std::endl;

        mutex.post();
		return;
	}

	int n_classes_old = 0;
	if (scores_buffer.size()>0)
		n_classes_old = scores_buffer.front().size();

	if (n_classes_old>0 && n_classes!=n_classes_old)
		scores_buffer.clear();

	scores_buffer.push_back(*bot);

	while (scores_buffer.size()>buffer_size)
		scores_buffer.pop_front();

	if (scores_buffer.size()<1)
	{
		std::cout << "There is a problem with buffer_size in streaming mode." << std::endl;
		mutex.post();
		return;
	}

	vector<double> class_avg(n_classes,0.0);
	vector<int> class_votes(n_classes,0);

	for (list<Bottle>::iterator score_itr=scores_buffer.begin(); score_itr!=scores_buffer.end(); score_itr++)
	{
		double max_score=-1000.0;
		int max_idx;
		for (int class_idx=0; class_idx<n_classes; class_idx++)
		{
			double s=score_itr->get(class_idx).asList()->get(1).asDouble();
			class_avg[class_idx]+=s;
			if(s>max_score)
			{
				max_score=s;
				max_idx=class_idx;
			}
		}

		class_votes[max_idx]++;
	}

	double max_avg=-10000.0;
	double max_votes=-10000.0;
	int max_avg_idx;
	int max_votes_idx;
	for(int class_idx=0; class_idx<n_classes; class_idx++)
	{
		class_avg[class_idx]=class_avg[class_idx]/n_classes;
		if(class_avg[class_idx]>max_avg)
		{
			max_avg=class_avg[class_idx];
			max_avg_idx=class_idx;
		}
		if(class_votes[class_idx]>max_votes)
		{
			max_votes=class_votes[class_idx];
			max_votes_idx=class_idx;
		}
	}

	predicted_class = scores_buffer.front().get(max_avg_idx).asList()->get(0).asString().c_str();
	if (max_votes/scores_buffer.size()<0.2)
		predicted_class = "?";

	cout << "Scores: " << endl;
	for (int i=0; i<n_classes; i++)
		cout << "[" << scores_buffer.front().get(i).asList()->get(0).asString().c_str() << "]: " << class_avg[i] << " "<< class_votes[i] << endl;
	cout << endl << endl;

	draw_hist(class_votes);

	mutex.post();
}

bool ScorerThread::clear_hist()
{
	mutex.wait();

	scores_buffer.clear();

	mutex.post();

	return true;
}

void ScorerThread::draw_hist(vector<int> bins)
{

	if (port_out_confidence.getOutputCount()>0)
	{

		int n_bins = bins.size();

        if (n_bins==0)
        {
            ImageOf<PixelRgb> img_zero;
            img_zero.resize(confidence_width,confidence_height);
            cvZero(img_zero.getIplImage());

            port_out_confidence.write(img_zero);

            std::cout << "still printing" << std::endl;
            return;
        }

		// normalize bins in [0,1]
		vector<float> norm_bins(n_bins,0);
		int max_votes_sum=0;
		for(int bin_idx=0; bin_idx<n_bins; bin_idx++)
			max_votes_sum += bins[bin_idx];
		for(int bin_idx=0; bin_idx<n_bins; bin_idx++)
			norm_bins[bin_idx] = (float)bins[bin_idx]/(float)max_votes_sum;

		// init image
		ImageOf<PixelRgb> img_hist;
		img_hist.resize(confidence_width,confidence_height);
		img_hist.zero();
		cv::Mat img_hist_mat = cv::cvarrToMat(img_hist.getIplImage());
		cv::Mat img_text_mat = cv::Mat::zeros(confidence_width, confidence_height, CV_8UC3);

		// setup bin dimensions
		int max_height = (int)img_hist.height()*0.9;
		int min_height = img_hist.height()-20;
		int width = (int)img_hist.width()/n_bins;

		// go!
		for (int bin_idx=0; bin_idx<n_bins; bin_idx++)
		{
			int height=img_hist.height()-int((float)max_height*(float)norm_bins[bin_idx]);
			if (height>min_height)
				height=min_height;

			cv::rectangle(img_hist_mat,cv::Point(bin_idx*width,height),cv::Point((bin_idx+1)*width,min_height),
					histColorsCode[bin_idx%(int)histColorsCode.size()],CV_FILLED);
		}
		for (int bin_idx=0; bin_idx<n_bins; bin_idx++)
		{
			cv::putText(img_text_mat,scores_buffer.front().get(bin_idx).asList()->get(0).asString().c_str(),
					cv::Point(img_hist.height()-min_height, bin_idx*width+width/2),
					cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255,255,255), 2);
		}
		transpose(img_text_mat, img_text_mat);
		flip(img_text_mat, img_text_mat, 0);
		img_hist_mat = img_hist_mat + img_text_mat;

		port_out_confidence.write(img_hist);
	}

}

bool ScorerThread::set_buffer_size(int _bsize)
{
	if (_bsize<=0)
		return false;

	mutex.wait();
	if (_bsize!=buffer_size)
	{
		scores_buffer.clear();
	}
	buffer_size = _bsize;

	mutex.post();

	return true;
}

bool ScorerThread::get_buffer_size(int &_bsize)
{
	mutex.wait();
	_bsize = buffer_size;
	mutex.post();

	return true;
}

bool ScorerThread::get_predicted_class(string &_predicted_class)
{
	mutex.wait();
	_predicted_class = predicted_class;
	mutex.post();

	return true;
}

bool ScorerThread::execReq(const Bottle &command, Bottle &reply)
{
	switch(command.get(0).asVocab())
	{
	default:
		return false;
	}
}

void ScorerThread::interrupt()
{
	mutex.wait();
	port_in_scores.interrupt();
	port_out_confidence.interrupt();
	mutex.post();
}

bool ScorerThread::releaseThread()
{
	mutex.wait();
	port_in_scores.close();
	port_out_confidence.close();
	mutex.post();

	return true;
}
