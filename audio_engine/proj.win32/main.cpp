//本程序，由by桔皮沙拉编写，
//qq1598058687
#include "OGDirector.h"
#include "AudioEngine.h"
#include "OGFileUtils.h" 
USING_OG;

int main(int, char **argv)
{

	Director::getInstance()->init();
	/*std::string path = FileUtils::getInstance()->fullPathForFilename("0013.mp3");
	printf("fullPathForFilename 返回: [%s]\n", path.c_str());
	printf("path 长度: %zu\n", path.length());
	printf("isFileExist: %d\n", FileUtils::getInstance()->isFileExist(path)); */

	 id = AudioEngine::play2d("0013.mp3");
	printf("play2d 返回 id=%d\n", id);
	if (id == AudioEngine::INVALID_AUDIO_ID) {
		printf("play2d 失败\n");
	}
	Director::getInstance()->mainLoop();
	
}