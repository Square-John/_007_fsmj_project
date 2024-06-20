import base64, configparser, time, uuid, zlib
from concurrent.futures import as_completed, ThreadPoolExecutor

import requests
import sys
from tqdm import tqdm


# 读取配置文件
config = configparser.ConfigParser(interpolation=None)
config.read('config.ini', encoding='utf-8')
# 获取cookie
minutes_cookie = config.get('Cookies', 'minutes_cookie')
# 从键盘获取文件路径

# 命令行参数通过 sys.argv 列表获取，其中 sys.argv[0] 是脚本名称
if len(sys.argv) > 1:
    # 获取第一个参数（索引为1，因为索引0是脚本名）
    file_path = sys.argv[1]
else:
    print("未提供文件路径，将退出程序\n")
    sys.exit()
    
# 获取代理设置
use_proxy = config.get('代理设置', '是否使用代理（是/否）')
proxy_address = config.get('代理设置', '代理地址')
if use_proxy == '是':
    proxies = {
        'http': proxy_address,
        'https': proxy_address,
    }
else:
    proxies = None

class FeishuUploader:
    def __init__(self, file_path, cookie):
        self.file_path = file_path
        self.block_size = 2**20*4
        self.headers = {
            'user-agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36',
            'cookie': cookie,
            'bv-csrf-token': cookie[cookie.find('bv_csrf_token=') + len('bv_csrf_token='):cookie.find(';', cookie.find('bv_csrf_token='))],
            'referer': 'https://minutes.feishu.cn/minutes/home'
        }
        if len(self.headers.get('bv-csrf-token')) != 36:
            raise Exception("cookie中不包含bv_csrf_token，请确保从请求`list?size=20&`中获取！")

        self.upload_token = None
        self.vhid = None
        self.upload_id = None
        self.object_token = None

        with open(self.file_path, 'rb') as f:
            self.file_size = f.seek(0, 2)
            f.seek(0)
            self.file_header = base64.b64encode(f.read(512)).decode()

    def get_quota(self):
        file_info = f'{uuid.uuid1()}_{self.file_size}'
        quota_url = f'https://meetings.feishu.cn/minutes/api/quota?file_info[]={file_info}&language=zh_cn'
        quota_res = requests.get(quota_url, headers=self.headers, proxies=proxies).json()
        if quota_res['data']['has_quota'] == False:
            raise Exception("飞书妙记空间已满，请清理后重试！")
        self.upload_token = quota_res['data']['upload_token'][file_info]

    # 分片上传文件（预上传）
    # doc: https://open.feishu.cn/document/server-docs/docs/drive-v1/upload/multipart-upload-file-/upload_prepare
    def prepare_upload(self):
        file_name = self.file_path.split("\\")[-1]
        # 如果文件名中包含后缀，需要去掉后缀
        if '.' in file_name:
            file_name = file_name[:file_name.rfind('.')]
        prepare_url = f'https://meetings.feishu.cn/minutes/api/upload/prepare'
        json = {
            'name': file_name,
            'file_size': self.file_size,
            'file_header': self.file_header,
            'drive_upload': True,
            'upload_token': self.upload_token,
        }
        prepare_res = requests.post(prepare_url, headers=self.headers, proxies=proxies, json=json).json()
        self.vhid = prepare_res['data']['vhid']
        self.upload_id = prepare_res['data']['upload_id']
        self.object_token = prepare_res['data']['object_token']

    # 分片上传文件（上传分片）
    # doc: https://open.feishu.cn/document/server-docs/docs/drive-v1/upload/multipart-upload-file-/upload_part
    def upload_blocks(self):
        with open(self.file_path, 'rb') as f:
            f.seek(0)
            block_count = (self.file_size + self.block_size - 1) // self.block_size
            with ThreadPoolExecutor(max_workers=6) as executor:
                completed_threads = []
                with tqdm(total=block_count, unit='block') as progress_bar:
                    for i in range(block_count):
                        block_data = f.read(self.block_size)
                        block_size = len(block_data)
                        checksum = zlib.adler32(block_data) & 0xffffffff
                        upload_url = f'https://internal-api-space.feishu.cn/space/api/box/stream/upload/block?upload_id={self.upload_id}&seq={i}&size={block_size}&checksum={checksum}'
                        thread = executor.submit(requests.post, upload_url, headers=self.headers, proxies=proxies, data=block_data)
                        completed_threads.append(thread)
                    for thread in as_completed(completed_threads):
                        progress_bar.update(1)
    
    # 分片上传文件（完成上传）
    # doc: https://open.feishu.cn/document/server-docs/docs/drive-v1/upload/multipart-upload-file-/upload_finish
    def complete_upload(self):
        complete_url1 = f'https://internal-api-space.feishu.cn/space/api/box/upload/finish/'
        json = {
            'upload_id': self.upload_id,
            'num_blocks': (self.file_size + self.block_size - 1) // self.block_size,
            'vhid': self.vhid,
            'risk_detection_extra' : '{\"source_terminal\":1,\"file_operate_usage\":3,\"locale\":\"zh_cn\"}'
        }
        resp = requests.post(complete_url1, headers=self.headers, proxies=proxies, json=json).json()
        # print(resp)

        complete_url2 = f'https://meetings.feishu.cn/minutes/api/upload/finish'
        json = {
            'auto_transcribe': True,
            'language': 'mixed',
            'num_blocks': (self.file_size + self.block_size - 1) // self.block_size,
            'upload_id': self.upload_id,
            'vhid': self.vhid,
            'upload_token': self.upload_token,
            'object_token': self.object_token,
        }
        resp = requests.post(complete_url2, headers=self.headers, proxies=proxies, json=json).json()
        # print(resp)


    def upload(self):
        self.get_quota()
        self.prepare_upload()
        self.upload_blocks()
        self.complete_upload()


if __name__ == '__main__':

    uploader = FeishuUploader(file_path, minutes_cookie)
    uploader.upload()
