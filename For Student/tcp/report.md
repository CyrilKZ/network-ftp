## Socket Programming 报告

2017013596 谭昊天



### server

#### 流程

#####启动

server根据控制台参数在指定根目录下启动（默认为/tmp），并绑定指定端口（默认为21）进行监听。

#####监听

当监听到客户端连接时，新建__子进程__，在子进程中使用`accept()`函数生成与客户端之间的connection（这个connection仅用于传输命令），同时建立结构体`Session`的实例`ssn`，记录该connection的socket标识符、登录状态、data socket标识符、本次需要返回给客户端的数据等等信息。

#####命令处理

客户端向服务器发送命令，服务器通过一系列操作将命令格式化并转化为`Command`结构体实例`cmd`，该结构体记录了本次命令的标题和参数，根据标题调用不同的函数进行处理，处理后设置`ssn`并向客户端返回数据。

本服务器支持以下命令：USER，PASS，RETR，STOR，REST，ABOR，QUIT，SYST，TYPE，PORT，PASV，MKD，CWD，PWD，LIST，RMD，RNFR，DELE，RNTO。

本服务器对命令的顺序有以下限制：

* 任何时候都可以使用QUIT和ABOR来断开连接，以下几条不考虑命令为QUIT和ABOR的情况；
* 在使用其他命令前，必须先使用USER，再使用PASS进行登录验证（本服务器只支持anonymous登录）；
* 使用RETR或STOR的前一个命令必须是PORT或PASV，在这之前还可以使用REST来设置上传、下载的offset以实现断点续传；
* LIST的前一个命令必须是PORT或PASV；
* PORT或PASV的下一个命令必须是LIST、RETR或STOR；
* RNFR的下一个命令必须是RNTO，RNTO的前一个命令必须是RNFR；
* TYPE只支持binary模式。

当客户端发送PASV命令后，服务器将在收到下载、上传请求之前将passive socket打开并开始监听；而收到PORT命令后，会等到下载、上传命令实际发送之后再开始试图连接至客户端端口。

当客户端发送QUIT或ABOR之后，服务器将断开connection（包括命令connection和数据connection），成功断开后释放内存并结束进程。客户端意外断线时服务端也会做相同的处理。

#### 代码实现

##### Linux标准库函数

* `chroot`：该函数利用了Linux本身的chroot设置了服务器主进程的根目录，防止服务器的任何用户进程访问根目录以外的目录，同时也方便了对其他路径函数的调用，但因此服务器需要sudo权限；
* `chdir`，`mkdir`，`getcwd`：配合chroot来切换路径、建立文件夹和查询路径；
* `rename`：对文件或目录进行重命名。

##### LIST的实现

在刚开始实现时，准备用`popen`函数打开Linux终端，在终端中运行"ls -l"命令，并将输出流导入进data connection中。但在实现过程中发现，由于用chroot更改了当前进程的根目录，而在新的根目录下没有"ls"等命令的定义，因此之前的想法是无效的。最后利用`opendir`，`readdir`和`stat`等函数实现了对目录下每一个子节点信息的读取和字符串化。



### Client

#### MyFTP

##### FTP命令

没有使用python的FTP库和PyQt的网络部分，从Socket开始写起完成了一套FTP协议。支持命令包括USER，PASS，RETR，STOR，REST，ABOR，QUIT，SYST，TYPE，PORT，PASV，MKD，CWD，PWD，LIST，RMD，RNFR，DELE，RNTO。

##### 使用FTP

在完成这些命令的基础上，实现了封装好的登录操作、上传/下载操作、目录操作等常用FTP功能。并使用了Python的Logger库以实现向外部接口提交debug信息。

#### GUI

##### 实现

使用PyQt5实现了一个较为简单但是操作便捷、功能较全的GUI。其中利用了QThread和QMutex实现多线程和线程锁，使得客户端一次只会和服务器共同完成一个任务，同时在完成这一任务时客户端不会出现“假死”的情况。

调用了上述MyFTP来进行文件操作，同时用一个Handler来接收Logger的信息，该Handler同时利用了Qt信号槽以便在一个文本框中随时更新相应的debug信息。

实现了文件列表的可视化，可以通过单击来选中某个列表中的文件，或双击进入某个目录。

##### 功能

用户在登录界面输入服务器地址、用户名、密码后点击loggin按钮登录。登录成功后可进入主界面，在主界面中可以进行文件操作。

在上传文件、新建目录之前，需要在文本框中输入新文件、新目录名；在修改文件名时，需要先在文本框中输入文件的新名称。

在进行了操作之后，客户端都会向服务器发送LIST和PWD命令，更新文件列表的显示和当前路径的显示。

文件的上传和下载可以通过按钮来切换PASV或PORT模式。

不支持自动的断点续传。