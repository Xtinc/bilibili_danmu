# bilibili_danmu
这是一个使用boost beast链接bilibili直播弹幕服务器并能抓取弹幕的程序。它解析bilibili私有的WSS协议，但协议规则是解包反推，故不保证完全准确。它也能做简单的NLP分词。
- 占用小，性能好，一台个人PC支持链接多数房间，1k左右QPS无压力
- 统计链接房间的弹幕，弹幕相关信息，一段时间内弹幕信息均值
- 链接过程中记录的弹幕所有记录以SQLITE数据库保存
- 支持JIEBA分词工具，能完成简单的NLP任务
- 生成统计信息文件，支持GNUPLOT绘图分析
- 支持短线重连，异常处理完善，不会无原因崩溃
## 使用方法
终端中启动 bilibili_danmu --roomid 510，房间号可以在B站分享网页中查看
![image](https://user-images.githubusercontent.com/39898895/162627291-de982f35-6dc4-450d-9d79-412c27902c79.png)
