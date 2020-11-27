#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <array>
#include <limits>
#include <pthread.h>

#include "libsitara-video.h"
#include "nix_console.h"

std::mutex    			g_lock;
std::condition_variable g_check;
std::atomic<bool> 		g_done{ false };

std::array<svl::VideoCaptureWrapper*, 4>	cap_mas		{ nullptr, nullptr, nullptr, nullptr };
std::array<svl::VideoDisplay*, 3>			disp_mas	{ nullptr, nullptr, nullptr};

int getNumFromCmd(int max, int min, char *args)
{
	char *tok;
	int num_of_cap = 0;

	int args_num = ::get_token_num(args, cm_delim);
	if (args_num != 1)
	{
		printf("error\n");
		return std::numeric_limits<int>::min();
	}
	else
	{
		num_of_cap = ::atoi(::strtok(args, cm_delim));

		if(num_of_cap > max)
			num_of_cap = max;
		else if(num_of_cap < min)
			num_of_cap =min;
	}

	return num_of_cap;
}

int cm_start_cap(char *args)
{
	int num_of_cap = getNumFromCmd(3, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(cap_mas[num_of_cap])
		cap_mas[num_of_cap]->start();

	printf("start capture %d\n", num_of_cap);
}

int cm_stop_cap(char *args)
{
	int num_of_cap = getNumFromCmd(3, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(cap_mas[num_of_cap])
		cap_mas[num_of_cap]->stop();

	printf("stop capture %d\n", num_of_cap);
}

int cm_on_display(char *args)
{
	int num_of_cap = getNumFromCmd(2, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(disp_mas[num_of_cap])
		disp_mas[num_of_cap]->on();

	printf("on display %d\n", num_of_cap);
}

int cm_of_display(char *args)
{
	int num_of_cap = getNumFromCmd(2, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(disp_mas[num_of_cap])
		disp_mas[num_of_cap]->off();

	printf("off display %d\n", num_of_cap);
}

int cm_start_disp(char *args)
{
	int num_of_cap = getNumFromCmd(2, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(disp_mas[num_of_cap])
		disp_mas[num_of_cap]->start();

	printf("start display %d\n", num_of_cap);
}

int cm_stop_disp(char *args)
{
	int num_of_cap = getNumFromCmd(2, 0, args);

	if(num_of_cap == std::numeric_limits<int>::min())
		return -1;

	if(disp_mas[num_of_cap])
		disp_mas[num_of_cap]->stop();

	printf("stop display %d\n", num_of_cap);
}


void threadConsole(const int id, std::string& str_, std::array<svl::VideoDisplay*, 3> &mas, std::array<svl::VideoCaptureWrapper*, 4> &cap)
{
	int id_ = id;

	unsigned tik = 0;
	for(auto wtf: mas)
	{
		if(wtf)
		{
			wtf->start();
			wtf->on();
			//cap[tik++]->start();
		}

	}

	::registerAllCommands();
	::workWithConsole();

	tik = 0;
	for(auto wtf: mas)
	{
		cap[tik++]->stop();
		//std::this_thread::sleep_for(std::chrono::milliseconds(50));
		if(wtf)
			wtf->stop();
	}

	g_done = true;
	std::cout << "threadConsole id'"<< id_ <<"' ended" << std::endl;

	//g_check.notify_one();
	//g_check.notify_all();
	std::unique_lock<std::mutex> locker(g_lock);
	std::notify_all_at_thread_exit(g_check, std::move(locker));
}

int main(int argc, char *argv[])
{
	svl::ListOfV4l2Dev::DataAboutV4l2DeV	lst_dev_v4l2(svl::VideoCapture::getListDev());

	svl::DRMvisual::getInstance().initOmap(kms::PixelFormat::NV12);

#define TEST_VIDEO_COUNT  (3)

#if TEST_VIDEO_COUNT>=1
	int tik = 0;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												,true, true, 9, false
												);
	disp_mas[tik] = new svl::VideoDisplay(cap_mas[tik]);
	disp_mas[tik]->setViewDisplayRect	(VideoEncLib::Rectangle(0, 0, 704/2, 280));
	disp_mas[tik]->setViewVideoRect		(VideoEncLib::Rectangle(0, 0, 704,   280));

#endif
#if TEST_VIDEO_COUNT>=2
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												,true, true, 9, false
												);
	disp_mas[tik] = new svl::VideoDisplay(cap_mas[tik]);
	disp_mas[tik]->setViewDisplayRect	(VideoEncLib::Rectangle(704/2, 0, 704/2,280));
	disp_mas[tik]->setViewVideoRect		(VideoEncLib::Rectangle(0,     0, 704,  280));
#endif
#if TEST_VIDEO_COUNT>=3
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												,true, true, 9, false
												);
	disp_mas[tik] = new svl::VideoDisplay(cap_mas[tik]);
	disp_mas[tik]->setViewDisplayRect	(VideoEncLib::Rectangle(0,		280,   704/2,	280));
	disp_mas[tik]->setViewVideoRect		(VideoEncLib::Rectangle(0,		0,     704,		280));
#endif

	int thr_id{0};
	std::string string_from_console;

	std::thread thr_console(::threadConsole, thr_id, std::ref(string_from_console), std::ref(disp_mas), std::ref(cap_mas));
	++thr_id;

	thr_console.join();

	std::cout << "programm successfully ended" << std::endl;

	return 0;
}
