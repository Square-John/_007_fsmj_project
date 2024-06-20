#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_PATH_LEN 1024
#define FILE_NAME_LEN 512
#define CMD_LEN 1024 * 3
#define CONF_PAHT "_02_lib/config.ini"

/**
 * 函数功能：将指定文件夹中的内容上传到飞书妙记
 */
int uploader(void) {
  // 步骤1：从键盘获取一个文件路径
  char filePath[FILE_PATH_LEN];
  printf("请输入一个文件路径：");
  fgets(filePath, FILE_PATH_LEN, stdin);
  printf("您输入的文件路径是：%s\n", filePath);
  // 将\替换为/
  for (int i = 0; i < strlen(filePath); i++) {
    if (filePath[i] == '\\') {
      filePath[i] = '/';
    }
  }
  // 将结尾换行符去除
  filePath[strlen(filePath) - 1] = '\0';

  // 步骤2：遍历文件里面的后缀为.mp4的文件
  DIR *dir;

  if ((dir = opendir(filePath)) == NULL) {
    perror("打开文件夹失败");
    return EXIT_FAILURE;
  }

  struct dirent *entry;
  int count = 0;
  while ((entry = readdir(dir)) != NULL) {
    // 遍历目录中后缀为.mp4的文件
    if (strstr(entry->d_name, ".mp4") != NULL) {
      printf("找到一个文件：%s\n", entry->d_name);
      // 将filePath和entry->d_name拼接成一个完整的文件路径
      char filePathAndName[FILE_PATH_LEN + FILE_NAME_LEN];
      strcpy(filePathAndName, filePath);
      strcat(filePathAndName, "/");
      strcat(filePathAndName, entry->d_name);
      //将filePathAndName的/替换为
      for (int i = 0; i < strlen(filePathAndName); i++) {
        if (filePathAndName[i] == '/') {
          filePathAndName[i] = '\\';
        }
      }
      count++;
      // 打印filePathAndName
      printf("第[%d]个完整文件路径为：%s\n", count, filePathAndName);

      // 将filePathAndName上传到飞书妙记中
      // 打印将第count个文件entry->d_name上传到飞书妙记
      printf("将第[%d]个文件%s上传到飞书妙记\n", count, entry->d_name);
      // 调用_02_lib/feishu_uploader.py
      char cmd[CMD_LEN];
      sprintf(cmd, "python _02_lib/feishu_uploader.py \"%s\" \"%s\"",
              filePathAndName, CONF_PAHT);
              printf("cmd:%s\n", cmd);
      system(cmd);

      //   count++;
    }
  }
  printf("共有[%d]个文件成功上传至飞书妙记\n", count);

  // 关闭目录
  closedir(dir);

  return EXIT_SUCCESS;
}

/**
 * 函数功能：下载飞书妙记中的字幕
 */
void downloader(void) {
  // 步骤1：从键盘获取一个文件路径
  char filePath[FILE_PATH_LEN];
  printf("请输入一个文件路径：");
  fgets(filePath, FILE_PATH_LEN, stdin);
  printf("您输入的文件路径是：%s\n", filePath);
  // 将\替换为/
  for (int i = 0; i < strlen(filePath); i++) {
    if (filePath[i] == '\\') {
      filePath[i] = '/';
    }
  }
  // 将结尾换行符去除
  filePath[strlen(filePath) - 1] = '\0';
  char cmd[CMD_LEN];
  sprintf(cmd, "python _02_lib/feishu_downloader.py \"%s\" \"%s\"", filePath,CONF_PAHT);
  system(cmd);
}

int main(int argc, char const *argv[])
{
    //提示用户选择功能，1：上传，2：下载
    printf("请选择功能：1：上传，2：下载\n");
    int select;
    scanf("%d",&select);
    //清除标准输入缓冲区
    while (getchar() != '\n'){};
    switch (select)
    {
    case 1:
        uploader();
        break;
    case 2:
        downloader();
        break;
    default:
        printf("请选择正确的功能\n");
        break;
    }
    return 0;
}

