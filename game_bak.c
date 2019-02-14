

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h> 
#include <stdio.h>
#include <linux/input.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <math.h>

#define BGCOLOR 0X00696969 //背景颜色
#define WIDTH 100  //方块宽度
#define ROW 4   //行数
#define COLUMN 4  //列数
#define GAP 5  //方块间间隔
#define BEGIN_X 190 //游戏矩阵开始处坐标
#define BEGIN_Y 30  //游戏矩阵开始处坐标

void draw_point(int x,int y,int color);//画点
void daw_mtrix(int x ,int y,int row,int column);//画游戏矩阵
void clear();//清屏
void draw_BMP(int x0,int y0,const char *bmpname);//画游戏图片
void draw_over_BMP();//画game_over 图片
//const char* getBmpBaseNum(int n);//根据数字获取图片路径
enum finger_move get_finger_move_direction();//获取划动方向 
void add_random_num();//在随机位置 添加随机数(2 或4)
void init_game();  //初始化
void begin_game();  //进行一次游戏


void move_up();//处理上划操作
void move_down();//处理下划操作
void move_right();//处理右划操作
void move_left();//处理下划操作
bool check_over();//检测是否结束游戏

int *p=NULL;
int *plcd;
bool merge=false;//全局变量，记录没次滑动是否产生合并

//全局数组，记录当前每个格子上面的数值
int nums[4][4]=
{
	0 , 0 , 0 , 0 ,
	0 , 0 , 0 , 0 ,
	0 , 0 , 0 , 0 ,               
	0 , 0 , 0 , 0 
};

//枚举类型，记录手指划动的方向
enum finger_move
{
	MOVE_UP = 1,//上
	MOVE_DOWN=2,//下
	MOVE_LEFT=3,//左
	MOVE_RIGHT=4,//右
};
	
void main()
{
	int fd=open("/dev/fb0",O_RDWR);
	if(fd<=0)
	{
		perror("error: ");
		return;
	}
	
	p=mmap(NULL,480*800*4, PROT_WRITE,MAP_SHARED,fd, 0);
	plcd=p;
	
	init_game();
	begin_game();
	
	sleep(1);
	clear();
	munmap(p,400*800*4);
	close(fd);
}


//开始游戏
void begin_game()
{
	
	while(!check_over())
	{
		enum finger_move mv = get_finger_move_direction();
		if(mv==MOVE_UP)
			move_up();
		else if (mv==MOVE_LEFT)
			move_left();	
		else if (mv==MOVE_DOWN)
			move_down();
		else if (mv==MOVE_RIGHT)
			move_right();

		if(merge)
		{
			add_random_num();
			
			daw_mtrix(BEGIN_X,BEGIN_Y,ROW,COLUMN);
		}
	}
	draw_over_BMP();
}

/*
功能:获取手指移动方向
返回值:枚举类型，代表四个方向
*/
enum finger_move get_finger_move_direction()
{
	int fd;
	enum finger_move mv;

	//fd = open("/dev/event0", O_RDONLY);
	fd = open("/dev/input/event0", O_RDONLY);
	if (fd == -1)
	{
		perror("open event0 error!");
	}

	struct input_event ev;
	int x1, y1; //滑动过程中第一个点的坐标值
	int x2,y2; //滑动过程中最后一个点的坐标值

	x1 = -1; 
	y1 = -1;
	
	while (1)
	{
		int r = read(fd, &ev, sizeof(ev));
		if (r == sizeof(ev))
		{
			if (ev.type == EV_ABS && ev.code == ABS_X)
			{
				if (x1 == -1)
					x1 = ev.value;
				x2 = ev.value;
			}

			if (ev.type == EV_ABS && ev.code == ABS_Y)
			{
				if (y1 == -1)
					y1 = ev.value;
				y2 = ev.value;
			}

			//手指弹起
			if (ev.type == EV_KEY && ev.code == BTN_TOUCH && ev.value == 0)
			//if (ev.type == EV_ABS && ev.code == ABS_PRESSURE && ev.value == 0)
			{
				//手指起点在游戏区域外
				if(x1<180||x1>600||y1<30||y1>450)
				{
					x1=-1;
					y1=-1;
				}

				if(x2<190)
				{
					x2=190;
				}
				if(x2>605)
				{
					x2=605;
				}
				if(y2<30)
				{
					y2=30;
				}
				if(y2>445)
				{
					y2=445;
				}
				
				
				int delta_x = abs(x2-x1);
				int delta_y = abs(y2-y1);

				if (delta_x >2*delta_y)
				{
					if ((x2 > x1)&&delta_x>80)
					{
						mv = MOVE_RIGHT;
					}
					else if((x1>x2)&&delta_x>80)
					{
						mv = MOVE_LEFT;
					}
					break;
					
				} else if (delta_y > 2 * delta_x)
				{
					if ((y2 > y1)&&delta_y>80)
					{
						mv = MOVE_UP;
					}
					else if((y1 > y2)&&delta_y>80)
					{
						mv = MOVE_DOWN;
					}
					break;
				}
				else //判定为滑动失效
				{
					x1 = -1;
					y1 = -1;
				}
			}
		}
	
	}

	
	close(fd);

	return mv;
}

/*
	功能:在随机位置 添加随机数(2 或4)
	返回值:无
*/
void add_random_num()
{
	int flag =0,pos=0;
	srand( (int)time(NULL));
	int x =0 ,y = 0;
	int r[16];
	int c[16];
	int i,j,k = 0;
	/*判断哪些位置为空*/
	for(i=0;i<4;i++)
	{
    		for(j=0;j<4;j++)
	    	{
	        	if(nums[i][j]==0)
	        	{
	            		r[k]=i;//保存第k+1个空格的x坐标
	            		c[k]=j;
	            		k++;//保存空位的个数
	        	}
	    	}
	}
	if(k!=0)//有空位
	{
    	pos = rand()%k;//一共k个空格，随机选择一个。//rand()%k的范围[0 ,k-1]
		x = r[pos];//第pos+1个空格的x坐标
		y = c[pos];
		nums[x][y] = rand()%20>1?2:4;
	}
}

/*
	功能:清空屏幕
	返回值:无
*/
void clear()
{
	//p=plcd;
	int x ,y;
	for(y=0;y<480;y++)
	{
		for(x=0;x<800;x++)
		{
			draw_point( x, y, BGCOLOR);
		}
	}
	
}

/*
    功能:	在指定位置画一个指定颜色的点
    参数:	x,y: 坐标; color: 颜色值
    返回值:无
*/
void draw_point(int x,int y,int color)
{
	p=plcd;
	*(p+y*800+x)=color;
}



/*
	功能:	画游戏主面板矩阵
	参数:  x,y:矩阵左上角的坐标; row.column:行列数目
	返回值:无
*/
void daw_mtrix(int x ,int y,int row,int column)//画16个bmp图片->游戏矩阵
{
	int m,n; 
	char bmpPath[10] = {0};
		
	for(n=0;n<column;n++)
	{
		for(m=0;m<row;m++)
		{
			//drawBMP(n*(WIDTH+GAP)+x,m*(WIDTH+GAP)+y, getBmpBaseNum(nums[m][n]));//由下面三行代替
			//     各子的坐标								获取图片
			sprintf(bmpPath,"%d.bmp",nums[m][n]);
			//strcat(bmpPath,".bmp");
			draw_BMP(n*(WIDTH+GAP)+x,m*(WIDTH+GAP)+y,bmpPath);
		}
	}
		
}

/*
	功能: 画图片
	参数: x,y:图片的位置，*bmpname :图片路径
	返回值:无
*/
void draw_BMP(int x0,int y0,const char *bmpname)//画一个bmp图片
{
	int  fd;
	unsigned char pixs[100*100*3];
	int i = 0;

	fd = open(bmpname, O_RDONLY);
	if (fd == -1)
	{
		perror("open failed:");
		return ;
	}

	lseek(fd, 54, SEEK_SET);
	read(fd, pixs, 100*100*3);
	close(fd);

	int x, y;
	
	for (y = 0; y < 100; y++)
	{
		//读一行
		for (x = 0; x < 100; x++)
		{
			unsigned char b,g,r;
			int color;
			b = pixs[i++];
			g = pixs[i++];
			r = pixs[i++];

			color = (r << 16) | (g << 8) | b;
			draw_point(x0 + x, y0 + 99 - y, color);
		}
	}
	
}

/*
	功能:根据传进来的数字n 获得对应的图片的路径
	参数: n :当前n 数值，其值必定为2  的n  次方
	返回值: 数值n  对应的图片的路径

const char* getBmpBaseNum(int n)
{
	const char* bmpPaths[17]=
	{
		"0.bmp",		"2.bmp",		"4.bmp",
		"8.bmp",		"16.bmp",	"32.bmp",
		"64.bmp",	"128.bmp",	"256.bmp",
		"512.bmp",   	"1024.bmp",   "2048.bmp",
		"4096.bmp",	"8192.bmp", 	"16384.bmp",
		"32768.bmp", "65535.bmp"
	};
	
	//利用对数运算求出数字n  在bmps[]数组中对应的bmpPath  位置
	int index=(int)(log10(n)/log10(2));
	//返回路径
	return bmpPaths[index];	
}
*/


/*
	功能:初始化
	 返回值:无
*/
void init_game()
{
	clear();
	memset(nums,0,sizeof(nums));     //把数组清零
	add_random_num();      	//添加随机数
	add_random_num();
	daw_mtrix(BEGIN_X,BEGIN_Y,ROW,COLUMN);		//设置游戏的开始坐标以及行数和列数
}

/*
	功能:画游戏结束时的图片
	返回值:无
*/
void draw_over_BMP()
{
	int  fd;
	unsigned char pixs[480*800*3];
	int i = 0;

	fd = open("game_over.bmp", O_RDONLY);
	if (fd == -1)
	{
		perror("open failed:");
		return ;
	}

	lseek(fd, 54, SEEK_SET);
	read(fd, pixs, 480*800*3);
	close(fd);

	int x, y;
	for (y = 0; y < 480; y++)
	{
		//读一行
		for (x = 0; x < 800; x++)
		{
			unsigned char b,g,r;
			int color;
			b = pixs[i++];
			g = pixs[i++];
			r = pixs[i++];

			color = (r << 16) | (g << 8) | b;
			draw_point( x,479 - y, color);
		}
	}
		
}

/*
	 功能:上滑时逻辑处理
	 返回值:无
*/
void move_up() 
{
	merge=false;
	int i,j,k;
	for(i=0;i<4;i++)//4列，所以循环四次
	{
		for(j=0;j<4;j++)//处理一列
		{
			for(k=j+1;k<4;k++)
			{
				if(nums[k][i]>0)//
				{
					if(nums[j][i]==0)
					{
							nums[j][i]=nums[k][i];
							nums[k][i]=0;
							merge=true;
					}
					else if(nums[j][i]==nums[k][i])
					{
							nums[j][i]=(nums[j][i]*2);
							nums[k][i]=0;
							merge=true;
					}
					break;
				}
			}
			}
	}
}

//下滑
void move_down()
{
	merge=false;
	int y,x,x1;
	for(y=0;y<4;y++)
	{
    	for(x=3;x>=0;x--)
		{
			for(x1=x-1;x1>=0;x1--)
			{
        		if(nums[x1][y]>0)
				{
	    			if(nums[x][y]<=0)
					{
            			nums[x][y]=(nums[x1][y]);
            			nums[x1][y]=0;
            			x++;
            			merge=true;
        			}else if(nums[x][y]==nums[x1][y])
        			{
            			nums[x][y]=(nums[x][y]*2);
            			nums[x1][y]=0;
            			merge=true;
        			}
        			break;
    			}
    		}
       	}
    }
}

//左滑
void move_left()  
{
	merge=false;
   	int x,y,y1;
    for(x=0;x<4;x++)
	{
      	for(y=0;y<4;y++)
		{
            for(y1=y+1;y1<4;y1++)
			{
                if(nums[x][y1]>0)
				{
                    if(nums[x][y]<=0)
					{
                        nums[x][y]=(nums[x][y1]);
                        nums[x][y1]=0;
                        y--;
                        merge=true;
                   	 }else if(nums[x][y]==nums[x][y1])
                   	 {
                        nums[x][y]=(nums[x][y]*2);
                        nums[x][y1]=0;       
                        merge=true;
                    }
                    break;
               }
            }
        }
   	}
}

//右滑
void move_right() 
{
	merge=false;
	int x,y,y1;
    for(x=0;x<4;x++)//有四行
	{
        for(y=3;y>=0;y--)//处理一行
		{
            for(y1=y-1;y1>=0;y1--)
			{
                if(nums[x][y1]>0)
				{
                    if(nums[x][y]<=0)
					{
            			nums[x][y]=(nums[x][y1]);
            			nums[x][y1]=(0);
            			y++;
            			merge=true;
        			}else if(nums[x][y]==nums[x][y1])
        			{
            			nums[x][y]=(nums[x][y]*2);
            			nums[x][y1]=0; 
            			merge=true;
        			}
        			break;
                }
            }
       	}
    }
}

/*
	功能:检查是否结束游戏
	返回值:布尔值
*/
bool check_over()
{	
	int x,y;
	for(y=0;y<4;y++)
	{	
		for (x=0;x<4;x++)
		{	
		
			if(nums[x][y]==0)
			{
				return false;
			}
			if(x>0 && nums[x][y]==nums[x-1][y])
			{
				return false;//这个if语句可去掉
			}
			if(x<3 && nums[x][y]==nums[x+1][y])
			{
				return false;
			}

			if(y>0 && nums[x][y]==nums[x][y-1])
			{
				return false;//这个if语句可去掉
			}
			if(y<3 && nums[x][y]==nums[x][y+1])
			{
				return false;
			}
		}
	}
	return true;  
}


