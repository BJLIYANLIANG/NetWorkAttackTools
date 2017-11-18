#!/bin/bash

#设置一些参数，免得改来改去麻烦
IF=wlan0
APP=a.out
MO="苟利国家生死以 岂因祸福避趋之 垂死病中惊坐起 谈笑风生又一年 处江泽之远则忧其民 稻花香里说丰年 听取人生经验 天若有情天亦老 我为长者续一秒 百壶且试开怀抱 熟悉西方那一套 长使英雄泪满襟 你们还是太年轻 在天愿作比翼鸟 人生经验还太少 千金散尽还复来 叫你闷声发大财 莫笑农家腊酒浑 弄出一个大新闻 黄沙百战穿金甲 按照香港基本法 敢同恶鬼争高下 不向霸王让寸分 靡靡之音 轻歌曼舞 江出扬州泽万民 习得文武近太平 一颗赛艇 二院视察 三个代表 四次起身 五可奉告 六月水柜 七因祸福 八门外语 九十大寿 十全长者"
ARG="文字 空格隔开 最多32字符 一个汉字算3个"

/bin/echo "开始关闭其他同源进程"
/usr/bin/killall -9 $APP


/bin/echo "修改网卡模式"
/sbin/ifconfig $IF down
/sbin/iwconfig $IF mode Monitor
/sbin/ifconfig $IF up


/bin/echo "开始运行程序"
/home/pi/NetWorkAttackTools/Beacon_Attack/$APP $IF $ARG 1>/dev/null