#include "base/OGDirector.h"
#include "AudioEngine.h"
#include "OGFileUtils.h"
#include <string>
#include <iostream>
#include <conio.h> 


OG_BEGIN
  

Director::Director()
{
	
}

Director::~Director()
{
	//free();
}

void Director::free() {
	// ★ 退出前清理音频引擎
	
	OG_SAFE_RELEASE(_scheduler);
}

 

void Director::init()
{ 
	_scheduler = new Scheduler;
	if (!AudioEngine::lazyInit()) {
		printf("AudioEngine lazyInit 失败！OpenAL 可能没初始化成功\n");
		return;
	}
	printf("AudioEngine lazyInit 成功\n");
}


static int  i = 0,j=0;

void Director::mainLoop()
{
	std::string cmd;
	while (true)
	{
		_scheduler->update(0.2f); 

		std::this_thread::sleep_for(std::chrono::milliseconds(10));

 		if (_kbhit())
 		{
 			int ch = _getch();
 
 		//	忽略回车、换行、空格等无效字符
 				if (ch == '\r' || ch == '\n' || ch == ' ' || ch == 0 || ch == 224)
 				{
 					//224 是方向键的前导字节，也忽略
 				}
 				else
 				{
 					switch (ch)
 					{
 					case '0':
						OG::AudioEngine::end();
 						return;
						
 					case '1':
 					{
 						AudioEngine::stopAll();
 						std::string path = FileUtils::getInstance()->fullPathForFilename("111.mp3");
 						AudioEngine::play2d(path);
 						printf("play 0013.mp3\n");
 					}
 					break;
 					case '2':
 					{
 						AudioEngine::stopAll();
 						std::string path = FileUtils::getInstance()->fullPathForFilename("沙漠骆驼_展展与罗罗_沙漠骆驼.mp3");
 						AudioEngine::play2d(path);
 						printf("play 沙漠骆驼_展展与罗罗_沙漠骆驼.mp3\n");
 					}
 					break;
 					case '3':{ 
 						auto t = AudioEngine::getGlobalPitch() + 0.1;
 					 	AudioEngine::setGlobalPitch(t);
 						printf("按4:%f\n", t);
 					}
 							 break;
 					case '4': {
 						
 						auto t = AudioEngine::getGlobalPitch() - 0.1;
 						AudioEngine::setGlobalPitch(t);
 						printf("按4:%f\n",t);
 					}
 					break;
 					case 'q': return;
 					case 's': AudioEngine::stopAll(); printf("stopAll\n"); break;
 					case 'p': AudioEngine::pauseAll(); printf("pauseAll\n"); break;
 					case 'r': AudioEngine::resumeAll(); printf("resumeAll\n"); break;
 					case 'f':
 						printf("freeze 3s...\n");
 						std::this_thread::sleep_for(std::chrono::seconds(3));
 						printf("resume\n");
 						break;
 					case 'i':
 						printf("state=%d, currTime=%f, duration=%f\n",
 							(int)AudioEngine::getState(id),
 							AudioEngine::getCurrentTime(id),
 							AudioEngine::getDuration(id));
 						break;
 					}
 				}
 
 		}
 

	///	阻塞等输入，输入前不返回
// 			if (!std::getline(std::cin, cmd)) break;
// 		if (cmd.empty()) continue;
// 
// 		if (cmd == "q") break;
// 		else if (cmd == "s") AudioEngine::stopAll();
// 		else if (cmd == "p") AudioEngine::pauseAll();
// 		else if (cmd == "r") AudioEngine::resumeAll();
// 		else if (cmd == "1") {
// 			auto t = AudioEngine::getGlobalPitch() + 1;
// 			AudioEngine::setGlobalPitch(t);
// 
// 		}
// 		else if (cmd == "2") {
// 			auto t = AudioEngine::getGlobalPitch() - 0.1;
// 			AudioEngine::setGlobalPitch(t);
// 		}
// 		else if (cmd == "a")
// 		{
// 			std::string path = FileUtils::getInstance()->fullPathForFilename("0013.mp3");
// 			AudioEngine::play2d(path);
// 		}
// 		else if (cmd == "freeze") {
// 			printf("冻结 3 秒...\n");
// 			std::this_thread::sleep_for(std::chrono::seconds(3));
// 			printf("恢复\n");
// 		}
// 		else if (cmd == "info") {
// 			printf("state=%d, currTime=%f, duration=%f\n",
// 				(int)AudioEngine::getState(0),
// 				AudioEngine::getCurrentTime(0),
// 				AudioEngine::getDuration(0));
// 		}
 	}
}

OG_END


