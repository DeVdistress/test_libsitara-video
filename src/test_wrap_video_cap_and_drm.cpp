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

void threadConsole(const int id, std::string& str_)
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

void threadVideo(const int id, svl::VideoCaptureWrapper& cap, const int num, const svl::DRMvisual::ListOfPlanesDef &lst_plans)
{
	int id_ = id;
	int num_ = num;
	//cap.startCapture();
#if(0)
	cap.start();
#endif

	unsigned tik__ = 0;
	bool first_shoot = true;
	VideoEncLib::Rectangle rec_d(0,0,0,0);
	VideoEncLib::Rectangle rec_v(0,0,0,0);

	bool flag = true;

	while(true)
	{
#if(1)
		if(!cap.isRun())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
#endif
		VideoEncLib::IFrameBuffer *ptr = nullptr;
		//cap.getFrame();
		ptr = cap.waitFrame();

		if(ptr == nullptr)
			continue;

#if(1)
		auto ptr_cur_plane = cap.getPlane();

		if(first_shoot)
		{
			switch (num_)
			{
				case 0:
					rec_d = VideoEncLib::Rectangle(0,		0,		704/2,	280);
					rec_v = VideoEncLib::Rectangle(0,		0,		704,	280);
					break;
				case 1:
					rec_d = VideoEncLib::Rectangle(704/2,	0,		704/2,	280);
					rec_v = VideoEncLib::Rectangle(0,		0,  	704,	280);
					break;
				case 2:
					rec_d = VideoEncLib::Rectangle(0,		280,   704/2,	280);
					rec_v = VideoEncLib::Rectangle(0,		0,     704,		280);
					break;

				default:
					break;
			}

			if (num_<3 && !cap.getUseDisplayed())
				svl::DRMvisual::getInstance().initPlane((ptr_cur_plane)?ptr_cur_plane:lst_plans[num_], ptr, rec_d, rec_v);

			first_shoot = false;
		}
		else
		{
#if(1)
			if(num_==0  && !cap.getUseDisplayed())
			{
				if(flag)
				{
					if((rec_d.y < (768-280)) && (rec_d.x < (1024-704/2)))
						rec_d.y +=1, rec_d.x +=1;
					else
					{
						flag = false;
					}
				}
				else
				{
					if((rec_d.y > 0) && (rec_d.x > 0))
						rec_d.y -=1, rec_d.x -=1;
					else
					{
						flag = true;
					}
				}

				svl::DRMvisual::getInstance().addFbIdToPlaneAndChangeGeometry(
																				(ptr_cur_plane)?ptr_cur_plane:lst_plans[num_], ptr,
																				rec_d, rec_v
																			);
			}
#endif
#if(0)
			if(num_==1  && !cap.getUseDisplayed())
			{
				if(rec_d.y < (768-280))
					rec_d.y +=2;
				else
					rec_d.y = 0;

				svl::DRMvisual::getInstance().addFbIdToPlaneAndChangeGeometry(
																				(ptr_cur_plane)?ptr_cur_plane:lst_plans[num_], ptr,
																				rec_d, rec_v
																			);
			}
#endif
#if(1)
			else if(num_==2  && !cap.getUseDisplayed())
			{


				if(flag)
				{
					if(rec_d.x < (1024-704/2))
						rec_d.x+=4;
					else
					{
						flag = false;
					}
				}
				else
				{
					if(rec_d.x > 0)
						rec_d.x-=4;
					else
					{
						flag = true;
					}
				}

				svl::DRMvisual::getInstance().addFbIdToPlaneAndChangeGeometry(
																				(ptr_cur_plane)?ptr_cur_plane:lst_plans[num_], ptr,
																				rec_d, rec_v
																			);
			}
#endif
			else if (num_<3 && !cap.getUseDisplayed())
			{
				svl::DRMvisual::getInstance().addFbIdToPlane((ptr_cur_plane)?ptr_cur_plane:lst_plans[num_], ptr);
			}
		}
#endif
		if (!(tik__++%253))
		{
			std::cout << "-----------num='"<< num_ << "'-----------" << std::endl;
			std::cout << "tik='"<< tik__-1 << "'" << std::endl;
			std::cout << "y-size   ='"	<< ptr->yBuff()->size()		<< "'" << std::endl;
			std::cout << "y-dma-id ='"	<< ptr->yBuff()->dmaBuf()	<< "'" << std::endl;
			std::cout << "y-map    ='"	<< ptr->yBuff()->dataMap()	<< "'" << std::endl;

			std::cout << "uv-size   ='"	<< ptr->uvBuff()->size()	<< "'" << std::endl;
			std::cout << "uv-dma-id ='"	<< ptr->uvBuff()->dmaBuf()	<< "'" << std::endl;
			std::cout << "uv-map    ='"	<< ptr->uvBuff()->dataMap()	<< "'" << std::endl;
		}

		// for not moved plane
		std::this_thread::sleep_for(std::chrono::milliseconds(32));

		//cap.continueCapture();
		cap.endingProcessFrameNotify();

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

	//cap.stopCapture();
	cap.stop();

	std::cout << "threadVideo id='"<< id_ << "' ended" << std::endl;

	//g_check.notify_all();
	std::unique_lock<std::mutex> locker(g_lock);
	std::notify_all_at_thread_exit(g_check, std::move(locker));
}

int main(int argc, char *argv[])
{
	svl::ListOfV4l2Dev::DataAboutV4l2DeV lst_dev_v4l2(svl::VideoCapture::getListDev());

#if(1)
	kms::PixelFormat my_px_fmt = kms::PixelFormat::NV12;
#else
	kms::PixelFormat my_px_fmt = kms::PixelFormat::YUYV;
#endif

	svl::DRMvisual::getInstance().initOmap(my_px_fmt);
	const svl::DRMvisual::ListOfPlanesDef lst_plane(svl::VideoCapture::getAvailablePlanesOfDrm());

	std::array<svl::VideoCaptureWrapper*, 4> cap_mas {nullptr, nullptr, nullptr, nullptr};

#define TEST_VIDEO_COUNT  (4)

#if TEST_VIDEO_COUNT>=1
	int tik = 0;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik], true, true, 9, true
												, lst_plane[tik],
												VideoEncLib::Rectangle(0, 0, 704/2, 280),
												VideoEncLib::Rectangle(0, 0, 704,   280),
												false, true, false
												);
#endif
#if TEST_VIDEO_COUNT>=2
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik], true, true, 9, false
												, lst_plane[tik],
												VideoEncLib::Rectangle(704/2, 0, 704/2,280),
												VideoEncLib::Rectangle(0,     0, 704,  280),
												false, true, true
												);
#endif
#if TEST_VIDEO_COUNT>=3
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik], true, true, 9, false
												, lst_plane[tik],
												VideoEncLib::Rectangle(0,		280,   704/2,	280),
												VideoEncLib::Rectangle(0,		0,     704,		280),
												false, true, false
												);
#endif
#if TEST_VIDEO_COUNT>=4
	++tik;
	cap_mas[tik] = new svl::VideoCaptureWrapper(
												lst_dev_v4l2.str_[tik]
												);
#endif

	int thr_id{0};
	std::string string_from_console;

	std::thread thr_console(::threadConsole, thr_id, std::ref(string_from_console));
	++thr_id;

#if(0)
	for( int tik = 0; tik < cap_mas.size(); ++tik)
	{
		if(cap_mas[tik])
			cap_mas[tik]->start();
	}
#endif

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

#if(1)
	for( int tik = 0; tik < cap_mas.size(); ++tik)
	{
		if(cap_mas[tik])
			cap_mas[tik]->start();
	}
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
