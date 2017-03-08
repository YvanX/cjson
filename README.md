# cjson
用ANSI C写的轻量级的JSON解析器


### 用途
#### JSON
 是一种轻量级的数据交换格式，[JSON](http://www.json.org/) 可以将 JavaScript 对象中表示的一组数据转换为字符串，可以在服务器-客户端之间传递数据
#### 本项目
 可以用来解析JSON字符串，生成相应的对象
 可以输入JSON对象到字符串
 提供了一些函数来操作JSON对象，比如查找、添加、删除、插入、替换等功能
#### 编译
 本体就json.c和json.h两个文件，很容易就能编译，所以不需要提供CMake支持或者Makefile
 也可以直接把这两个文件添加到项目中直接使用
 一个简单的编译方法
 ``` shell
 gcc -c json.c
 ar rcs libjson.a json.o
 ```
#### 测试
 ```
 gcc main.c -o test -L. -ljson
 ```
#### 后记
 只是练手的项目，并没有大规模测试
 只在测试过程中作用了[VLD](http://vld.codeplex.com/)来检测有没有内存泄漏

