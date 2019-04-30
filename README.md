# chat_room
### 程序说明
- 程序运行环境均为Linux操作系统
- 程序包括server.c和client.c两个文件
    - server.c已部署到服务器上，创建守护进程
    - client.c默认通信对象为服务器
    - 已初始创建三个测试账号，分别为：
        - 账号：ZWMDR，密码：zwmdr
        - 账号：ZCCNB，密码：zccnb
        - 账号：CYXNB，密码：cyxnb
        - 账号：Test，密码：test
        - **账号也可任意注册**
- 工程包含makefile文件，可直接通过makefile文件编译client与server两个程序
    - makefile文件已包含预编译头
