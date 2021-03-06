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

#include <pthread.h>

#include "libsitara-video.h"
#include "nix_console.h"

std::mutex    			g_lock;
std::condition_variable g_check;
std::atomic<bool> 		g_done{ false };

void threadConsole(const int id, std::string &str_)
{
	int id_ = id;

	::registerAllCommands();
	::workWithConsole();

	g_done = true;
	std::cout << "threadConsole id'"<< id_ <<"' ended" << std::endl;

	//g_check.notify_one();
	//g_check.notify_all();
	std::unique_lock<std::mutex> locker(g_lock);
	std::notify_all_at_thread_exit(g_check, std::move(locker));
}

void threadVideo(const int id, svl::VideoCapture &cap, const int num, const svl::DRMvisual::ListOfPlanesDef &lst_plans)
{
	int id_ = id;
	int num_ = num;
	cap.startCapture();
	//cap.start();

	unsigned tik = 0;
	bool first_shoot = true;
	svl::Rectangle<uint32_t> rec_d(0,0,0,0);
	svl::Rectangle<uint32_t> rec_v(0,0,0,0);

	while(true)
	{
		kms::OmapFramebuffer* ptr= cap.waitTheFrame();

		if(first_shoot)
		{
			switch (num_)
			{
				case 0:
					rec_d = svl::Rectangle<uint32_t>(0,     0,     704/2, 280);
					rec_v = svl::Rectangle<uint32_t>(0,     0,     704,   280);
					break;
				case 1:
					rec_d = svl::Rectangle<uint32_t>(704/2, 0,     704/2, 280);
					rec_v = svl::Rectangle<uint32_t>(4*100, 4*30,  704/2, 280/2);
					break;
				case 2:
					rec_d = svl::Rectangle<uint32_t>(0,     280,   704/2, 280);
					rec_v = svl::Rectangle<uint32_t>(0,     0,     704,   280);
					break;

				default:
					break;
			}

			if (num_<3)
				svl::DRMvisual::getInstance().initPlane(lst_plans[num_], ptr, rec_d, rec_v);

			first_shoot = false;
		}
		else
		{
			if (num_<3)
				svl::DRMvisual::getInstance().addFbIdToPlane(lst_plans[num_], ptr);
		}

		cap.continueCapture();
		//cap.endingProcessFrameNotify();

		{
		  std::unique_lock<std::mutex> locker(g_lock);

		  auto now_time = std::chrono::system_clock::now();

		  if( g_check.wait_until(locker, now_time + std::chrono::milliseconds(1), [](){ return 1; }))
		  {
			  if(g_done)
			  {
				  break;
			  }
		  }
		}
	}

	cap.stopCapture();
	//cap.stop();

	std::cout << "threadVideo id='"<< id_ << "' ended" << std::endl;

	//g_check.notify_all();
	std::unique_lock<std::mutex> locker(g_lock);
	std::notify_all_at_thread_exit(g_check, std::move(locker));
}

int main(int argc, char *argv[])
{
	svl::ListOfV4l2Dev::DataAboutV4l2DeV	lst_dev_v4l2(svl::VideoCapture::getListDev());

#if(1)
	kms::PixelFormat my_px_fmt = kms::PixelFormat::NV12;
#else
	kms::PixelFormat my_px_fmt = kms::PixelFormat::YUYV;
#endif

	svl::DRMvisual::getInstance().initOmap(my_px_fmt);
	const svl::DRMvisual::ListOfPlanesDef lst_plane(svl::VideoCapture::getAvailablePlanesOfDrm());

	std::array<svl::VideoCapture*, 4> cap_mas { nullptr, nullptr, nullptr, nullptr };

#define TEST_VIDEO_COUNT  (4)

#if TEST_VIDEO_COUNT>=1
	int tik = 0;
	cap_mas[tik] = new svl::VideoCapture( lst_dev_v4l2.str_[tik] , true, false, 8, false );
#endif
#if TEST_VIDEO_COUNT>=2
	++tik;
	cap_mas[tik] = new svl::VideoCapture( lst_dev_v4l2.str_[tik] , true, false, 8, false );
#endif
#if TEST_VIDEO_COUNT>=3
	++tik;
	cap_mas[tik] = new svl::VideoCapture( lst_dev_v4l2.str_[tik] , true, false, 8, false );
#endif
#if TEST_VIDEO_COUNT>=4
	++tik;
	cap_mas[tik] = new svl::VideoCapture( lst_dev_v4l2.str_[tik] , true, false, 8, false );
#endif

	int thr_id{0};
	std::string string_from_console;

	std::thread thr_console(::threadConsole, thr_id, std::ref(string_from_console));
	++thr_id;

	int num_of_cap = 0;
#if TEST_VIDEO_COUNT>=1
	std::thread thr_video1(::threadVideo, thr_id, std::ref(*cap_mas[num_of_cap]), num_of_cap, std::cref(lst_plane));
#endif
#if TEST_VIDEO_COUNT>=2
	++thr_id, ++num_of_cap;
	std::thread thr_video2(::threadVideo, thr_id, std::ref(*cap_mas[num_of_cap]), num_of_cap, std::cref(lst_plane));
#endif
#if TEST_VIDEO_COUNT>=3
	++thr_id, ++num_of_cap;
	std::thread thr_video3(::threadVideo, thr_id, std::ref(*cap_mas[num_of_cap]), num_of_cap, std::cref(lst_plane));
#endif
#if TEST_VIDEO_COUNT>=4
	++thr_id, ++num_of_cap;
	std::thread thr_video4(::threadVideo, thr_id, std::ref(*cap_mas[num_of_cap]), num_of_cap, std::cref(lst_plane));
#endif

	// enable killing thread
	int res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if (res != 0)
		std::cout << "error-'" << res << "' pthread_setcancelstate" << std::endl;

	pthread_t id_console_thread = thr_console.native_handle();

	thr_console.join();

#if TEST_VIDEO_COUNT>=1
	thr_video1.join();
#endif
#if TEST_VIDEO_COUNT>=2
	thr_video2.join();
#endif
#if TEST_VIDEO_COUNT>=3
	thr_video3.join();
#endif
#if TEST_VIDEO_COUNT>=4
	thr_video4.join();
#endif

	for( int tik = 0; tik < cap_mas.size(); ++tik)
	{
		if(cap_mas[tik])
		{
			delete cap_mas[tik];
			cap_mas[tik] = nullptr;
		}
	}

	std::cout << "programm successfully ended" << std::endl;

	return 0;
}
