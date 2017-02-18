//The Return
//Author: Philip Rushik
//License: Beerware
//
#include <stdint.h>			//Types
#include <fcntl.h>			//File RW flags
#include <linux/fb.h>		//fb_{var,fix}_screeninfo structures
#include <linux/input.h>	//input_event structure
#include <sys/mman.h>		//MMAP #defines
#include <sys/ioctl.h>		//IOCTL #defines
#include <sys/syscall.h>	//syscall #defines (SYS_xxx)
#include <time.h>			//timespec structure
#include <math.h>			//sin, cos, sincos, and atan2 prototypes and M_PI

//#define M_PI 3.14159
#define GRAVITY 0.27

//absolute value macro
#define ABS(a)  ((a)<0 ? -(a) : (a))

void end_loop();
void menu_loop();
void game_loop();

void (*loop)();

struct ship
{
	float x,y;
	float rot;
	float xsp,ysp;
	int loaded,reload;
	int thrust;
	int health,damage,arm,range;
	int charge;
	int land;
} user;

//Missiles
struct missile
{
	float x,y;
	float xsp,ysp;
	int arm,damage,life;
};

#define MISSILE_MAX 50
static struct missile missl[MISSILE_MAX];
static int msl_c=0;

//Asteroids
struct asteroid
{
	double x,y;
	double xsp,ysp;
	int size;
};

#define ASTEROID_MAX 50
static struct asteroid ast[ASTEROID_MAX];
static int ast_c=0;

//Targets
struct target
{
	double x,y;
	int alive;
};

struct target tgt[10];

//Turrets
struct turret
{
	double x,y;
	double rot;
	int loaded,reload;
	int health,damage,arm,range;
};

#define TURRET_MAX 5
static struct turret trt[TURRET_MAX];
static int trt_c;

//Tanks
struct tank
{
	double x,y;
	double xsp,ysp;
	float rot;
	int loaded,reload;
	int health,damage,arm,range;
};

#define TANK_MAX 10
static struct tank tnk[TANK_MAX];
static int tnk_c;

//Airships
struct balloon
{
	float x,y;
	float xsp,ysp;
	float rot;
	int loaded,reload;
	int float_health,gun_health;
};

#define BALLOON_MAX 10
static struct balloon air[BALLOON_MAX];
static int air_c;

//Scud Missiles
struct scud
{
	float x,y;
	float xsp,ysp;
	int arm,damage,life;
};

#define SCUD_MAX 30
static struct scud scd[SCUD_MAX];
static int scd_c;

//Special effects
struct effect
{
	float x,y;
	float xsp,ysp;
	int radius;
	int life;
	uint8_t r,g,b;
	void (*update)(struct effect *);
	void (*draw)(struct effect *);
};

#define EFFECT_MAX 100
static struct effect eff[EFFECT_MAX];
static int eff_c=0;

#define GAME_AST_FLAG 0x01	//Asteroids
#define GAME_TUR_FLAG 0x02	//Turrets
#define GAME_MSL_FLAG 0x04	//Scud Missiles
#define GAME_TGT_FLAG 0x08	//Targets
#define GAME_TNK_FLAG 0x10	//Tanks
#define GAME_AIR_FLAG 0x20	//Airships
#define GAME_ALL_FLAG 0x40	//Combination Game

#define GAME_END_FLAG 0x80	//Player is dead

struct game
{
	uint8_t flags;
	int wave;
	int kills, total;
	int end_flags;
	int end_timer;
} game;

extern long syscall_list(int, long *);

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
uint8_t *fbp = 0;

inline uint32_t pixel_color(uint8_t r, uint8_t g, uint8_t b)
{
	return (0<<24) | (r<<16) | (g<<8) | (b<<0);
}

uint32_t xor128()
{
	static uint32_t x = 123456789;
	static uint32_t y = 362436069;
	static uint32_t z = 521288629;
	static uint32_t w = 88675123;
	uint32_t t;

	t = x ^ (x << 11);
	x = y; y = z; z = w;

	return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}

void cleanup(int r)
{
	syscall1(SYS_exit,r);
}

static uint32_t *back_buffer;

void init_double_buffer()
{
	long mmap_args[]={(long)0, (long)( vinfo.yres_virtual * finfo.line_length), (long)(PROT_READ|PROT_WRITE), (long)(MAP_PRIVATE|MAP_ANONYMOUS), (long)(-1), (long)0};
	back_buffer = (uint32_t*)syscall_list(SYS_mmap,mmap_args);
}

void writeint(int fd, uint16_t num)
{
	char res[6];
	int len = 0;
	for (; num>0; len++)
	{
		res[len] = num%10+'0';
		num=num/10;
	}
	res[len] = 0; //null-terminating

	int i;
	for (i = len; i>=0; i--)
	{
		__syscall(SYS_write,fd,&res[i],1);
	}
}

static long gettime()
{
	static long last_ns;
	long ns;
	struct timespec ts;
	syscall2(SYS_clock_gettime,CLOCK_REALTIME,&ts);
	ns=ts.tv_nsec-last_ns;
	last_ns=ts.tv_nsec;
	return ns;
}

inline void draw(int x, int y, uint32_t pixel)
{
//	long int location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
//	*((uint32_t*)(fbp + location)) = pixel;
	long int location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;
	*((uint32_t*)((uint8_t*)back_buffer + location)) = pixel;

	return;
}

//Display the back buffer on the screen
inline void swap_buffers()
{
	int i;
	for (i=0;i<(vinfo.yres_virtual * finfo.line_length)/4;i++)
	{
		((uint32_t*)(fbp))[i] = back_buffer[i];
	}
}

//Draw a circle at (cx,cy) of radius radius
void draw_circle(double cx, double cy, int radius, uint32_t pixel)
{
	inline void plot4points(double cx, double cy, double x, double y, uint32_t pixel)
	{
		draw(cx + x, cy + y,pixel);
		draw(cx - x, cy + y,pixel);
		draw(cx + x, cy - y,pixel);
		draw(cx - x, cy - y,pixel);
	}

	inline void plot8points(double cx, double cy, double x, double y, uint32_t pixel)
	{
		plot4points(cx, cy, x, y,pixel);
		plot4points(cx, cy, y, x,pixel);
	}

	if (cy-radius<1 || cy+radius>=vinfo.yres)
		return;
	if (cx-radius<1 || cx+radius>=vinfo.xres)
		return;

	int error = -radius;
	float x = radius;
	float y = 0;

	while (x >= y)
	{
		plot8points(cx, cy, x, y, pixel);

		error += y;
		y++;
		error += y;

		if (error >= 0)
		{
			error += -x;
			x--;
			error += -x;
		}
	}
}

//Draw a line from (x1,y1) to (x2,y2)
void draw_line_fast(int x1, int y1, int x2, int y2, uint32_t pixel)
{
	if (y1<=0 || y2<=0 || y1>=vinfo.yres || y2>=vinfo.yres)
		return;
	if (x1<=0 || x2<=0 || x1>=vinfo.xres || x2>=vinfo.xres)
		return;

	int i,dx,dy,sdx,sdy,dxabs,dyabs,x,y,px,py;

	dx=x2-x1;      /* the horizontal distance of the line */
	dy=y2-y1;      /* the vertical distance of the line */
	dxabs=abs(dx);
	dyabs=abs(dy);
	sdx=sgn(dx);
	sdy=sgn(dy);
	x=dyabs>>1;
	y=dxabs>>1;
	px=x1;
	py=y1;

	if (dxabs>=dyabs) // the line is more horizontal than vertical
	{
		for(i=0;i<dxabs;i++)
		{
			y+=dyabs;
			if (y>=dxabs)
			{
				y-=dxabs;
				py+=sdy;
			}
			px+=sdx;
			draw(px,py,pixel);
		}
	}
	else // the line is more vertical than horizontal
	{
		for(i=0;i<dyabs;i++)
		{
			x+=dxabs;
			if (x>=dyabs)
			{
				x-=dyabs;
				px+=sdx;
			}
			py+=sdy;
			draw(px,py,pixel);
		}
	}
}

//Draw the ship on the screen
void draw_ship(struct ship *ship)
{
	if (ship->health<=0)
		return;

	double x1,y1,x2,y2,x3,y3;
	x1=cos((double)ship->rot);
	y1=sin((double)ship->rot);
	x2=cos((double)ship->rot+(M_PI*0.75));
	y2=sin((double)ship->rot+(M_PI*0.75));
	x3=cos((double)ship->rot-(M_PI*0.75));
	y3=sin((double)ship->rot-(M_PI*0.75));

	x1=x1*25;
	y1=y1*25;
	x2=x2*25;
	y2=y2*25;
	x3=x3*25;
	y3=y3*25;

	draw_line_fast(ship->x,ship->y,ship->x+x1,ship->y-y1,pixel_color(0xff,0x14,0x55));
	draw_line_fast(ship->x,ship->y,ship->x+x2,ship->y-y2,pixel_color(0xff,0x14,0x55));
	draw_line_fast(ship->x,ship->y,ship->x+x3,ship->y-y3,pixel_color(0xff,0x14,0x55));
	draw_line_fast(ship->x+x1,ship->y-y1,ship->x+x2,ship->y-y2,pixel_color(0xff,0x14,0x55));
	draw_line_fast(ship->x+x1,ship->y-y1,ship->x+x3,ship->y-y3,pixel_color(0xff,0x14,0x55));
}

//Draw the player's remaining health (a circle for each remaining hit)
void draw_user_health()
{
	int i;
	for (i=0;i<user.health;i++)
		draw_circle(50+(i*25),50,5,pixel_color(0xff,0xaa,0xaa));
}

//Draw the wave and score
void draw_hud()
{
	int i;
	for (i=0;i<game.wave;i++)
		draw_circle(vinfo.xres-50-((i%4)*25),50+((i/4)*25),5,pixel_color(0xff,0xff,0xff));
	for (i=0;i<game.total;i++)
		draw_line_fast(vinfo.xres-50-(i*4),20,vinfo.xres-50-((i+((i%5%4)-(i%5)))*4),30,pixel_color(0x55,0x00,0x00)); //Talley marks
	for (i=0;i<game.kills;i++)
		draw_line_fast(vinfo.xres-50-(i*4),20,vinfo.xres-50-((i+((i%5%4)-(i%5)))*4),30,pixel_color(0xaa,0x22,0x22)); //Talley marks
}

//Draw the wave and score after game end
void draw_end_hud()
{
	int i;
	for (i=0;i<game.wave;i++)
		draw_circle((vinfo.xres/2)+((((game.wave-1)%8)/2)*50)-((i%8)*50),(vinfo.yres/2)-50-((i/8)*50),15,pixel_color(0xff,0xff,0xff));
	for (i=0;i<game.total;i++)
		draw_line_fast((vinfo.xres/2)+(game.total/2*6)-(i*6),(vinfo.yres/2)-20,(vinfo.xres/2)+(game.total/2*6)-((i+((i%5%4)-(i%5)))*6),(vinfo.yres/2)+20,pixel_color(0xff,0x00,0x00)); //Talley marks
}

//Draw a turret on the screen
void draw_turret(struct turret *trt)
{
	if (trt->health<=0)
		return;

	double x1,y1,x2,y2,x3,y3;
	x1=cos((double)trt->rot);
	y1=sin((double)trt->rot);
	x2=cos((double)trt->rot+(M_PI*0.25));
	y2=sin((double)trt->rot+(M_PI*0.25));
	x3=cos((double)trt->rot-(M_PI*0.25));
	y3=sin((double)trt->rot-(M_PI*0.25));

	x1=x1*25;
	y1=y1*25;
	x2=x2*25;
	y2=y2*25;
	x3=x3*25;
	y3=y3*25;

	draw_line_fast(trt->x,trt->y,trt->x+x1,trt->y-y1,pixel_color(0x00,0x14,0xff));
	draw_line_fast(trt->x,trt->y,trt->x+x2,trt->y-y2,pixel_color(0x00,0x14,0xff));
	draw_line_fast(trt->x,trt->y,trt->x+x3,trt->y-y3,pixel_color(0x00,0x14,0xff));
	draw_line_fast(trt->x+(x1/2),trt->y-(y1/2),trt->x+x2,trt->y-y2,pixel_color(0x00,0x14,0xff));
	draw_line_fast(trt->x+(x1/2),trt->y-(y1/2),trt->x+x3,trt->y-y3,pixel_color(0x00,0x14,0xff));
}

//Draw a tank on the screen
void draw_tank(struct tank *tnk)
{
	if (tnk->health<=0)
		return;

	double x1,y1,x2,y2,x3,y3;
	x1=cos((double)tnk->rot);
	y1=sin((double)tnk->rot);
	x2=cos((double)tnk->rot+(M_PI*0.25));
	y2=sin((double)tnk->rot+(M_PI*0.25));
	x3=cos((double)tnk->rot-(M_PI*0.25));
	y3=sin((double)tnk->rot-(M_PI*0.25));

	x1=x1*25;
	y1=y1*25;
	x2=x2*25;
	y2=y2*25;
	x3=x3*25;
	y3=y3*25;

	draw_line_fast(tnk->x,tnk->y,tnk->x+x1,tnk->y-y1,pixel_color(0x00,0xff,0x14));
	draw_line_fast(tnk->x,tnk->y,tnk->x+x2,tnk->y-y2,pixel_color(0x00,0xff,0x14));
	draw_line_fast(tnk->x,tnk->y,tnk->x+x3,tnk->y-y3,pixel_color(0x00,0xff,0x14));
	draw_line_fast(tnk->x+(x1/2),tnk->y-(y1/2),tnk->x+x2,tnk->y-y2,pixel_color(0x00,0xff,0x14));
	draw_line_fast(tnk->x+(x1/2),tnk->y-(y1/2),tnk->x+x3,tnk->y-y3,pixel_color(0x00,0xff,0x14));

	draw_circle(tnk->x-10,tnk->y+5,7,pixel_color(0x00,0xff,0x14));
	draw_circle(tnk->x+10,tnk->y+5,7,pixel_color(0x00,0xff,0x14));
	draw_line_fast(tnk->x-10,tnk->y+5,tnk->x+10,tnk->y+5,pixel_color(0x00,0xff,0x14));
}

//Draw an airship on the screen
void draw_balloon(struct balloon *air)
{
	if (air->gun_health>0)
	{
		double x1,y1,x2,y2,x3,y3;
		x1=cos((double)air->rot);
		y1=sin((double)air->rot);
		x2=cos((double)air->rot+(M_PI*0.25));
		y2=sin((double)air->rot+(M_PI*0.25));
		x3=cos((double)air->rot-(M_PI*0.25));
		y3=sin((double)air->rot-(M_PI*0.25));

		x1=x1*25;
		y1=y1*25;
		x2=x2*25;
		y2=y2*25;
		x3=x3*25;
		y3=y3*25;

		draw_line_fast(air->x,air->y,air->x+x1,air->y-y1,pixel_color(0x14,0xff,0xff));
		draw_line_fast(air->x,air->y,air->x+x2,air->y-y2,pixel_color(0x14,0xff,0xff));
		draw_line_fast(air->x,air->y,air->x+x3,air->y-y3,pixel_color(0x14,0xff,0xff));
		draw_line_fast(air->x+(x1/2),air->y-(y1/2),air->x+x2,air->y-y2,pixel_color(0x14,0xff,0xff));
		draw_line_fast(air->x+(x1/2),air->y-(y1/2),air->x+x3,air->y-y3,pixel_color(0x14,0xff,0xff));
	}
	if (air->float_health>0)
	{
		draw_circle(air->x,air->y-80,25,pixel_color(0x14,0xff,0xff));
		draw_circle(air->x-10,air->y-90,5,pixel_color(0x28,0xff,0xff));
		draw_line_fast(air->x,air->y,air->x,air->y-55,pixel_color(0xaa,0xaa,0x14));
	}
}

void draw_balloon_menu(struct balloon *air,uint8_t fade)
{
	double x1,y1,x2,y2,x3,y3;
	x1=cos((double)air->rot);
	y1=sin((double)air->rot);
	x2=cos((double)air->rot+(M_PI*0.25));
	y2=sin((double)air->rot+(M_PI*0.25));
	x3=cos((double)air->rot-(M_PI*0.25));
	y3=sin((double)air->rot-(M_PI*0.25));

	x1=x1*25;
	y1=y1*25;
	x2=x2*25;
	y2=y2*25;
	x3=x3*25;
	y3=y3*25;

	draw_line_fast(air->x,air->y,air->x+x1,air->y-y1,pixel_color(0x00,0x19,0x19)*fade);
	draw_line_fast(air->x,air->y,air->x+x2,air->y-y2,pixel_color(0x00,0x19,0x19)*fade);
	draw_line_fast(air->x,air->y,air->x+x3,air->y-y3,pixel_color(0x00,0x19,0x19)*fade);
	draw_line_fast(air->x+(x1/2),air->y-(y1/2),air->x+x2,air->y-y2,pixel_color(0x00,0x19,0x19)*fade);
	draw_line_fast(air->x+(x1/2),air->y-(y1/2),air->x+x3,air->y-y3,pixel_color(0x00,0x19,0x19)*fade);

	draw_circle(air->x,air->y-80,25,pixel_color(0x00,0x19,0x19)*fade);
	draw_circle(air->x-10,air->y-90,5,pixel_color(0x05,0x19,0x19)*fade);
	draw_line_fast(air->x,air->y,air->x,air->y-55,pixel_color(0x17,0x17,0x00)*fade);
}

void draw_tank_menu(struct tank *tnk,uint8_t fade)
{
	double x1,y1,x2,y2,x3,y3;
	x1=cos((double)tnk->rot);
	y1=sin((double)tnk->rot);
	x2=cos((double)tnk->rot+(M_PI*0.25));
	y2=sin((double)tnk->rot+(M_PI*0.25));
	x3=cos((double)tnk->rot-(M_PI*0.25));
	y3=sin((double)tnk->rot-(M_PI*0.25));

	x1=x1*25;
	y1=y1*25;
	x2=x2*25;
	y2=y2*25;
	x3=x3*25;
	y3=y3*25;

	draw_line_fast(tnk->x,tnk->y,tnk->x+x1,tnk->y-y1,pixel_color(0x00,0x19,0x00)*fade);
	draw_line_fast(tnk->x,tnk->y,tnk->x+x2,tnk->y-y2,pixel_color(0x00,0x19,0x00)*fade);
	draw_line_fast(tnk->x,tnk->y,tnk->x+x3,tnk->y-y3,pixel_color(0x00,0x19,0x00)*fade);
	draw_line_fast(tnk->x+(x1/2),tnk->y-(y1/2),tnk->x+x2,tnk->y-y2,pixel_color(0x00,0x19,0x00)*fade);
	draw_line_fast(tnk->x+(x1/2),tnk->y-(y1/2),tnk->x+x3,tnk->y-y3,pixel_color(0x00,0x19,0x00)*fade);

	draw_circle(tnk->x-10,tnk->y+5,7,pixel_color(0x00,0x19,0x00)*fade);
	draw_circle(tnk->x+10,tnk->y+5,7,pixel_color(0x00,0x19,0x00)*fade);
	draw_line_fast(tnk->x-10,tnk->y+5,tnk->x+10,tnk->y+5,pixel_color(0x00,0x19,0x00)*fade);
}

void draw_turret_menu(struct turret *trt, uint8_t fade)
{
	double x1,y1,x2,y2,x3,y3;
	x1=cos((double)trt->rot);
	y1=sin((double)trt->rot);
	x2=cos((double)trt->rot+(M_PI*0.25));
	y2=sin((double)trt->rot+(M_PI*0.25));
	x3=cos((double)trt->rot-(M_PI*0.25));
	y3=sin((double)trt->rot-(M_PI*0.25));

	x1=x1*50;
	y1=y1*50;
	x2=x2*50;
	y2=y2*50;
	x3=x3*50;
	y3=y3*50;

	draw_line_fast(trt->x,trt->y+25,trt->x+x1,trt->y-y1+25,pixel_color(0x00,0x01,0x19)*fade);
	draw_line_fast(trt->x,trt->y+25,trt->x+x2,trt->y-y2+25,pixel_color(0x00,0x01,0x19)*fade);
	draw_line_fast(trt->x,trt->y+25,trt->x+x3,trt->y-y3+25,pixel_color(0x00,0x01,0x19)*fade);
	draw_line_fast(trt->x+(x1/2),trt->y-(y1/2)+25,trt->x+x2,trt->y-y2+25,pixel_color(0x00,0x01,0x19)*fade);
	draw_line_fast(trt->x+(x1/2),trt->y-(y1/2)+25,trt->x+x3,trt->y-y3+25,pixel_color(0x00,0x01,0x19)*fade);
}

inline void draw_missile(struct missile *obj)
{
	if (obj->arm>0)
		draw_circle(obj->x,obj->y,5,pixel_color(0xff,0xff,0x00));
	else
		draw_circle(obj->x,obj->y,5,pixel_color(0xff,0x00,0x00));
	draw_circle(obj->x-obj->xsp,obj->y-obj->ysp,5,pixel_color(0xff,0xff,0x00));

	//Missile
	draw_line_fast(obj->x,obj->y,obj->x-obj->xsp,obj->y-obj->ysp,pixel_color(0xff,0xff,0xff));
}

inline void draw_asteroid(struct asteroid *obj)
{
	switch (obj->size)
	{
		case 0:
			return;
		case 1:
			draw_circle(obj->x,obj->y,10,pixel_color(0xbb,0x88,0x77));
			break;
		case 2:
			draw_circle(obj->x,obj->y,20.0,pixel_color(0xbb,0x88,0x77));
			draw_circle(obj->x-5.0,obj->y-5.0,5.0,pixel_color(0xbb,0x99,0x88));
			break;
	}
}

void draw_asteroid_menu(struct asteroid *obj, uint8_t fade)
{
	draw_circle(obj->x,obj->y,20,pixel_color(0x19,0x14,0x14)*fade);
	draw_circle(obj->x-5,obj->y-5,5,pixel_color(0x14,0x14,0x14)*fade);
}

inline void draw_scud(struct scud *obj)
{
	if (obj->arm>0)
		draw_circle(obj->x+obj->xsp,obj->y+obj->ysp,2.5,pixel_color(0xff,0xff,0x00));
	else
		draw_circle(obj->x+obj->xsp,obj->y+obj->ysp,2.5,pixel_color(0xff,0x00,0x00));
	draw_circle(obj->x,obj->y,5,pixel_color(0xff,0xff,0x00));
	draw_circle(obj->x-obj->xsp,obj->y-obj->ysp,5,pixel_color(0xff,0xff,0x00));

	//Missile
	draw_line_fast(obj->x+obj->xsp,obj->y+obj->ysp,obj->x-obj->xsp,obj->y-obj->ysp,pixel_color(0xff,0xff,0xff));
}

void draw_target(struct target *obj)
{
	if (obj->alive==0)
		return;

	draw_circle(obj->x,obj->y,30,pixel_color(0x88,0x88,0x88));
	draw_circle(obj->x,obj->y,20,pixel_color(0xAA,0x00,0x00));
	draw_circle(obj->x,obj->y,10,pixel_color(0x88,0x88,0x88));
}

void draw_target_menu(struct target *obj,uint8_t fade)
{
	draw_circle(obj->x,obj->y,30,pixel_color(0x19,0x19,0x19)*fade);
	draw_circle(obj->x,obj->y,20,pixel_color(0x11,0x00,0x00)*fade);
	draw_circle(obj->x,obj->y,10,pixel_color(0x19,0x19,0x19)*fade);
}

void draw_exit_menu(uint8_t fade)
{
	uint32_t x=(vinfo.xres/2);
	uint32_t y=(vinfo.yres/2);
	draw_circle(x,y,30,pixel_color(0x19,0x00,0x00)*fade);
	draw_line_fast(x+15,y+15,x-15,y-15,pixel_color(0x19,0x00,0x00)*fade);
	draw_line_fast(x-15,y+15,x+15,y-15,pixel_color(0x19,0x00,0x00)*fade);
}

void draw_effect_explosion(struct effect *obj)
{
	draw_circle(obj->x,obj->y,obj->radius,pixel_color(obj->r,obj->g,obj->b));
}

void update_effect_explosion(struct effect *obj)
{
	obj->radius=(obj->radius+1)*1.5;
	obj->life+=-1;
}

void draw_effect_warn(struct effect *obj)
{
	if (obj->radius<4)
		return;

	draw_line_fast(obj->x,10,obj->x,50,pixel_color(obj->r,obj->g,obj->b));
	draw_line_fast(obj->x,10,obj->x-15,25,pixel_color(obj->r,obj->g,obj->b));
	draw_line_fast(obj->x,10,obj->x+15,25,pixel_color(obj->r,obj->g,obj->b));
}

void update_effect_warn(struct effect *obj)
{
	obj->radius=(obj->radius+1)%10;
	obj->life+=-1;
}

void draw_effect_tail(struct effect *obj)
{
	int i;
	float x, y, xsp, ysp;

	x=obj->x;
	y=obj->y;

	xsp=obj->xsp;
	ysp=obj->ysp;
	
	uint32_t color=pixel_color(0xFF,0xFF,0xFF);
	
	for (i=0;i<obj->radius;i++)
	{
		draw_line_fast(x,y,x-xsp,y-ysp,color);
		x+=-xsp;
		y+=-ysp;
		ysp+=-GRAVITY;
		color+=-pixel_color(0x11,0x11,0x11);
	}
}

void update_effect_tail(struct effect *obj)
{
	if (obj->radius<15)
		obj->radius+=1;

	obj->life+=-1;

	obj->x+=obj->xsp;
	obj->y+=obj->ysp;
	obj->ysp+=GRAVITY;
}

//Must be called every step for all ships, otherwise they don't move
void update_ship(struct ship *ship)
{
	if (ship->health<=0)
		return;

	ship->x+=ship->xsp;
	ship->y+=ship->ysp;

	ship->loaded+=-1;

	if (ship->y>=vinfo.yres-25)
	{
		if (ship->ysp>10 || ship->xsp>7)
			ship->health=0;
		ship->y=vinfo.yres-25;
		ship->ysp=0;
		ship->xsp=0;
		ship->rot=M_PI*0.5;
	}
	else
		ship->ysp+=GRAVITY;
}

void update_turret(struct turret *trt)
{
	if (trt->health<=0)
		return;

	if (trt->loaded<=0)
	{
		missl[msl_c].xsp=(cos(trt->rot)*15);
		missl[msl_c].ysp=-(sin(trt->rot)*15);
		missl[msl_c].x=trt->x+(cos(trt->rot)*30);
		missl[msl_c].y=trt->y-(sin(trt->rot)*30);
		missl[msl_c].arm=30;
		missl[msl_c].life=70;
		trt->loaded=100;

		eff[eff_c].life=70;
		eff[eff_c].x=missl[msl_c].x;
		eff[eff_c].y=missl[msl_c].y;
		eff[eff_c].xsp=missl[msl_c].xsp;
		eff[eff_c].ysp=missl[msl_c].ysp;
		eff[eff_c].r=0xff;
		eff[eff_c].g=0xaa;
		eff[eff_c].b=0xff;
		eff[eff_c].radius=0;
		eff[eff_c].draw=&draw_effect_tail;
		eff[eff_c].update=&update_effect_tail;

		eff_c=(eff_c+1)%EFFECT_MAX;

		msl_c=(msl_c+1)%MISSILE_MAX;
	}
	else
		trt->loaded+=-1;

	trt->rot=atan2(trt->x-user.x,trt->y-user.y)+(M_PI*0.5);
}

void update_tank(struct tank *tnk)
{
	if (tnk->health<=0)
		return;

	if (tnk->loaded<=0)
	{
		missl[msl_c].xsp=(cos(tnk->rot)*15);
		missl[msl_c].ysp=-(sin(tnk->rot)*15);
		missl[msl_c].x=tnk->x+(cos(tnk->rot)*30);
		missl[msl_c].y=tnk->y-(sin(tnk->rot)*30);
		missl[msl_c].arm=(ABS(tnk->y-user.y)>200)?30:10;
		missl[msl_c].life=missl[msl_c].arm+75;
		tnk->loaded=100;

		//Tail effect
		eff[eff_c].life=70;
		eff[eff_c].x=missl[msl_c].x;
		eff[eff_c].y=missl[msl_c].y;
		eff[eff_c].xsp=missl[msl_c].xsp;
		eff[eff_c].ysp=missl[msl_c].ysp;
		eff[eff_c].r=0xff;
		eff[eff_c].g=0xaa;
		eff[eff_c].b=0xff;
		eff[eff_c].radius=0;
		eff[eff_c].draw=&draw_effect_tail;
		eff[eff_c].update=&update_effect_tail;

		eff_c=(eff_c+1)%EFFECT_MAX;

		msl_c=(msl_c+1)%MISSILE_MAX;
	}
	else
		tnk->loaded+=-1;

	tnk->rot=atan2(tnk->x-user.x,tnk->y-user.y)+(M_PI*0.5);
	tnk->x+=tnk->xsp;
	if (tnk->x>user.x+400)
		tnk->xsp+=-0.25;
	else if (tnk->x<user.x-400)
		tnk->xsp+=0.25;
	else
		tnk->xsp+=(tnk->xsp<0)?0.25:-0.25;
}

void update_balloon(struct balloon *air)
{
	if (air->gun_health>0)
	{
		if (air->loaded<=0)
		{
			missl[msl_c].xsp=(cos(air->rot)*15);
			missl[msl_c].ysp=-(sin(air->rot)*15);
			missl[msl_c].x=air->x+(cos(air->rot)*30);
			missl[msl_c].y=air->y-(sin(air->rot)*30);
			missl[msl_c].arm=(ABS(air->y-user.y)>200)?30:10;
			missl[msl_c].life=missl[msl_c].arm+75;
			air->loaded=100;

			msl_c=(msl_c+1)%MISSILE_MAX;
		}
		else
			air->loaded+=-1;

		air->rot=atan2(air->x-user.x,air->y-user.y)+(M_PI*0.5);
	}

	if (air->y<vinfo.yres-25)
		air->x+=air->xsp;

	if (air->float_health>0)
	{
		if (air->x>user.x+400)
			air->xsp+=-0.01;
		else if (air->x<user.x-400)
			air->xsp+=0.01;
		else
			air->xsp+=(air->xsp<0)?0.01:-0.01;
	}
	else
	{
		if (air->gun_health>0)
			if (air->y<vinfo.yres-25)
			{
				air->y+=air->ysp;
				air->ysp+=GRAVITY;
			}
			else
			{
				if (air->ysp>10 || air->xsp>7)
				{
					air->gun_health+=-1;
					game.kills+=1;
				}
				air->y=vinfo.yres-25;
				air->ysp=0;
				air->xsp=0;
			}
	}
}

//Must be called for every missile
void update_missile(struct missile *obj)
{
	obj->x+=obj->xsp;
	obj->y+=obj->ysp;

	obj->ysp+=GRAVITY;

	obj->life--;
	obj->arm--;
}

void update_scud(struct scud *obj)
{
	obj->x+=obj->xsp;
	obj->y+=obj->ysp;

	obj->ysp+=GRAVITY;

	obj->life--;
	obj->arm--;
}

void update_asteroid(struct asteroid *obj)
{
	obj->x+=obj->xsp;
	obj->y+=obj->ysp;

	obj->ysp+=GRAVITY;

	if (obj->y>=vinfo.yres-25)
	{
		obj->y=vinfo.yres-25;
		obj->ysp=-(obj->ysp*0.85);
	}

	if (obj->x>vinfo.xres)
	{
		obj->x=obj->y;
		obj->y=0;
	}
	else if (obj->x<0)
	{
		obj->x=vinfo.xres-obj->y;
		obj->y=0;
	}
}

inline int target_x_missile(struct target *tgt, struct missile *msl)
{
	if (msl->arm>0)
		return 0;

	if ((ABS(tgt->x-msl->x)<25) && (ABS(tgt->y-msl->y)<25) && tgt->alive)
		return 1;
	return 0;
}

inline int scud_x_missile(struct scud *scd, struct missile *msl)
{
	if (msl->arm>0 && scd->arm>0)
		return 0;

	if ((ABS(scd->x-msl->x)<15) && (ABS(scd->y-msl->y)<15) && scd->life>0)
		return 1;
	return 0;
}

inline int asteroid_x_missile(struct asteroid *ast, struct missile *msl)
{
	if (msl->arm>0)
		return 0;

	if (ast->size==1)
		if ((ABS(ast->x-msl->x)<15) && (ABS(ast->y-msl->y)<15))
			return 1;
	if (ast->size==2)
		if ((ABS(ast->x-msl->x)<25) && (ABS(ast->y-msl->y)<25))
			return 1;
	return 0;
}

inline int tank_x_missile(struct tank *tnk, struct missile *msl)
{
	if (msl->arm>0)
		return 0;

	if (tnk->health>0)
		if ((ABS(tnk->x-msl->x)<25) && (ABS(tnk->y-msl->y)<25))
			return 1;
	return 0;
}

inline int balloon_gun_x_missile(struct balloon *air, struct missile *msl)
{
	if (msl->arm>0)
		return 0;

	if (air->gun_health>0)
		if ((ABS(air->x-msl->x)<25) && (ABS(air->y-msl->y)<25))
			return 1;
	return 0;
}

inline int balloon_float_x_missile(struct balloon *air, struct missile *msl)
{
	if (msl->arm>0)
		return 0;

	if (air->float_health>0)
		if ((ABS(air->x-msl->x)<25) && (ABS((air->y-80)-msl->y)<25))
			return 1;
	return 0;
}

inline int turret_x_missile(struct turret *trt, struct missile *msl)
{
	if (msl->arm>0)
		return 0;

	if (trt->health>0)
		if ((ABS(trt->x-msl->x)<25) && (ABS(trt->y-msl->y)<25))
			return 1;
	return 0;
}

inline int asteroid_x_ship(struct asteroid *ast, struct ship *ship)
{
	if (ast->size==1)
		if ((ABS(ast->x-ship->x)<15) && (ABS(ast->y-ship->y)<15))
			return 1;
	if (ast->size==2)
		if ((ABS(ast->x-ship->x)<25) && (ABS(ast->y-ship->y)<25))
			return 1;
	return 0;
}

inline int ship_x_missile(struct ship *ship, struct missile *msl)
{
	if (msl->arm>0)
		return 0;

	if ((ABS(ship->x-msl->x)<25) && (ABS(ship->y-msl->y)<25) && ship->health>0)
		return 1;
	return 0;
}

uint8_t key[248];

int get_keys(int evfd)
{
	struct input_event ev;
	int r = syscall3(SYS_read,evfd,&ev,sizeof(struct input_event));
	if (r>0)
	{
		if (ev.type == 1)
		{
			key[ev.code]=ev.value;
		}
		return 1;
	}
	else
		return 0;
}

inline void clear()
{
	int x,y;

	//Clear Screen
	for (y=0;y<vinfo.yres;y++)
		for (x=0;x<vinfo.xres;x++)
		{
			draw(x,y,pixel_color(0x00,0x00,0x00));
		}
}

//Detect and respond to end of wave conditions (and death)
void game_condition()
{
	int i;

	//Check if end of wave conditions are met
	if (game.end_flags==game.flags)
	{
		game.wave+=1;
		game.kills=0;
		game.end_flags=0;

		if (game.flags&GAME_MSL_FLAG)
		{
		}
		if (game.flags&GAME_TUR_FLAG)
		{
			trt[0].x=vinfo.xres/2;
			trt[0].y=750;
			trt[0].health=1;
			trt[0].loaded=10;

			if (game.wave>=3)
			{
				trt[1].x=vinfo.xres/2-400;
				trt[1].y=750;
				trt[1].health=1;
				trt[1].loaded=20;

				trt[2].x=vinfo.xres/2+400;
				trt[2].y=750;
				trt[2].health=1;
				trt[2].loaded=xor128()%200;
			}

			if (game.wave>=5)
			{
				trt[3].x=vinfo.xres/2-600;
				trt[3].y=750;
				trt[3].health=1;
				trt[3].loaded=20;

				trt[4].x=vinfo.xres/2+600;
				trt[4].y=750;
				trt[4].health=1;
				trt[4].loaded=xor128()%200;
			}
		}
		if (game.flags&GAME_AIR_FLAG)
		{
			for (i=0;i<(game.wave%BALLOON_MAX);i++)
			{
				air[i].gun_health=(game.wave>5)?3:1;
				air[i].float_health=1;
				air[i].x=((xor128()%61)<30)?-25.0:vinfo.xres+25.0;
				air[i].y=xor128()%(vinfo.yres-160)+100;
				air[i].xsp=(xor128()%3)-1.5;
				air[i].loaded=xor128()%200;
			}
			air_c=i;
		}
		if (game.flags&GAME_TNK_FLAG)
		{
			for (i=0;i<(game.wave%TANK_MAX);i++)
			{
				tnk[i].health=(game.wave>3?2:1);
				tnk[i].x=xor128()%vinfo.xres;
				tnk[i].y=vinfo.yres-20;
				tnk[i].xsp=(xor128()%10)-5.0;
			}
			tnk_c=i;
		}
		if (game.flags&GAME_AST_FLAG)
		{
			for (i=0;i<game.wave;i++)
			{
				ast[i].size=2;
				ast[i].x=xor128()%vinfo.xres;
				ast[i].y=0;
				ast[i].xsp=(xor128()%10)-5.0;
				ast[i].ysp=0;
			}
			ast_c=i;
		}
	}
	if (user.health<=0)
	{
		if (game.end_timer<=0)
		{
			clear();
			loop=end_loop;//Show the death screen
		}
		game.end_flags=game.end_flags|GAME_END_FLAG;
		game.end_timer+=-1;
	}
}

void scud_control()
{
	int i;
	static scd_l=200; //interval to next scud

	//Create scuds
	scd_l+=-1;
	if (scd_l<=0)
	{
		scd_l=100-(10*game.wave);

		scd[scd_c].x=xor128()%vinfo.xres;
		scd[scd_c].y=4;
		scd[scd_c].ysp=-2;
		scd[scd_c].xsp=(xor128()%5)-2.5;
		scd[scd_c].life=110;
		scd[scd_c].arm=40;

		//Create tail
		eff[eff_c].life=110;
		eff[eff_c].x=scd[scd_c].x;
		eff[eff_c].y=scd[scd_c].y;
		eff[eff_c].xsp=scd[scd_c].xsp;
		eff[eff_c].ysp=scd[scd_c].ysp;
		eff[eff_c].r=0xff;
		eff[eff_c].g=0xff;
		eff[eff_c].b=0xff;
		eff[eff_c].radius=0;
		eff[eff_c].draw=&draw_effect_tail;
		eff[eff_c].update=&update_effect_tail;

		eff_c=(eff_c+1)%EFFECT_MAX;

		//Create warning effect
		eff[eff_c].life=20;
		eff[eff_c].radius=0;
		eff[eff_c].x=scd[scd_c].x+(scd[scd_c].xsp*10);
		eff[eff_c].y=10;
		eff[eff_c].r=0xff;
		eff[eff_c].g=0x00;
		eff[eff_c].b=0x00;
		eff[eff_c].draw=&draw_effect_warn;
		eff[eff_c].update=&update_effect_warn;

		eff_c=(eff_c+1)%EFFECT_MAX;

		scd_c=(scd_c+1)%SCUD_MAX;
	}
	for (i=0;i<SCUD_MAX;i++)
	{
		if (scd[i].life<=0)
			continue;

		draw_scud(&scd[i]);
		update_scud(&scd[i]);

		int j;
		for (j=0;j<MISSILE_MAX;j++)
		{
			if (missl[j].life>0 && scud_x_missile(&scd[i],&missl[j]))
			{
				game.kills+=1;
				game.total+=1;
				scd[i].life=0;
				missl[j].life=0;

				eff[eff_c].life=5;
				eff[eff_c].x=scd[i].x;
				eff[eff_c].y=scd[i].y;
				eff[eff_c].r=0xff;
				eff[eff_c].g=0xaa;
				eff[eff_c].b=0x11;
				eff[eff_c].radius=0;
				eff[eff_c].draw=&draw_effect_explosion;
				eff[eff_c].update=&update_effect_explosion;

				eff_c=(eff_c+1)%EFFECT_MAX;
			}
		}
		if (ship_x_missile(&user,(struct missile*)&scd[i]))
		{
			scd[i].life=0;
			user.health+=-1;

			eff[eff_c].life=5;
			eff[eff_c].radius=5;
			eff[eff_c].x=scd[i].x-scd[i].xsp;
			eff[eff_c].y=scd[i].y-scd[i].ysp;
			eff[eff_c].r=0xff;
			eff[eff_c].g=0xaa;
			eff[eff_c].b=0x11;
			eff[eff_c].draw=&draw_effect_explosion;
			eff[eff_c].update=&update_effect_explosion;

			eff_c=(eff_c+1)%EFFECT_MAX;
		}
	}
	if (game.kills>=game.wave*2)
		game.end_flags=game.end_flags|GAME_MSL_FLAG;
}

void turret_control()
{
	int num;	//number of turrets remaining

	num=0;
	int i;
	for (i=0;i<TURRET_MAX;i++)
	{
		draw_turret(&trt[i]);
		update_turret(&trt[i]);

		num+=trt[i].health;

		int j;
		for (j=0;j<MISSILE_MAX;j++)
			if (missl[j].life>0 && turret_x_missile(&trt[i],&missl[j]))
			{
				trt[i].health+=-1;
				missl[j].life=0;
				game.total+=1;

				eff[eff_c].life=10;
				eff[eff_c].x=trt[i].x;
				eff[eff_c].y=trt[i].y;
				eff[eff_c].r=0xff;
				eff[eff_c].g=0xaa;
				eff[eff_c].b=0x11;
				eff[eff_c].radius=0;
				eff[eff_c].draw=&draw_effect_explosion;
				eff[eff_c].update=&update_effect_explosion;

				eff_c=(eff_c+1)%EFFECT_MAX;
			}
	}

	if (num<=0)
	{
		game.end_flags=game.end_flags|GAME_TUR_FLAG;
	}
}

void balloon_control()
{
	int num;	//number of airships remaining

	num=0;

	int i;
	for (i=0;i<BALLOON_MAX;i++)
	{
		draw_balloon(&air[i]);
		update_balloon(&air[i]);

		num+=air[i].gun_health+air[i].float_health;

		int j;
		for (j=0;j<MISSILE_MAX;j++)
		{
			if (missl[j].life>0 && balloon_gun_x_missile(&air[i],&missl[j]))
			{
				air[i].gun_health+=-1;
				missl[j].life=0;
				game.kills+=1;
				game.total+=1;

				eff[eff_c].life=10;
				eff[eff_c].x=air[i].x;
				eff[eff_c].y=air[i].y;
				eff[eff_c].r=0xff;
				eff[eff_c].g=0xaa;
				eff[eff_c].b=0x11;
				eff[eff_c].radius=0;
				eff[eff_c].draw=&draw_effect_explosion;
				eff[eff_c].update=&update_effect_explosion;

				eff_c=(eff_c+1)%EFFECT_MAX;
			}
			if (missl[j].life>0 && balloon_float_x_missile(&air[i],&missl[j]))
			{
				air[i].float_health+=-1;
				missl[j].life=0;

				eff[eff_c].life=5;
				eff[eff_c].x=air[i].x;
				eff[eff_c].y=air[i].y-80;
				eff[eff_c].r=0x14;
				eff[eff_c].g=0xff;
				eff[eff_c].b=0xff;
				eff[eff_c].radius=25;
				eff[eff_c].draw=&draw_effect_explosion;
				eff[eff_c].update=&update_effect_explosion;

				eff_c=(eff_c+1)%EFFECT_MAX;
			}
		}
	}

	if (num<=0)
	{
		game.end_flags=game.end_flags|GAME_AIR_FLAG;
	}
}

void tank_control()
{
	int num;	//number of tanks remaining

	num=0;

	int i;
	for (i=0;i<TANK_MAX;i++)
	{
		draw_tank(&tnk[i]);
		update_tank(&tnk[i]);

		num+=tnk[i].health;

		int j;
		for (j=0;j<MISSILE_MAX;j++)
			if (missl[j].life>0 && tank_x_missile(&tnk[i],&missl[j]))
			{
				tnk[i].health+=-1;
				missl[j].life=0;

				eff[eff_c].life=10;
				eff[eff_c].x=tnk[i].x;
				eff[eff_c].y=tnk[i].y;
				eff[eff_c].r=0xff;
				eff[eff_c].g=0xaa;
				eff[eff_c].b=0x11;
				eff[eff_c].radius=0;
				eff[eff_c].draw=&draw_effect_explosion;
				eff[eff_c].update=&update_effect_explosion;

				eff_c=(eff_c+1)%EFFECT_MAX;

				game.kills+=1;
				game.total+=1;
			}
	}

	if (num<=0)
	{
		game.end_flags=game.end_flags|GAME_TNK_FLAG;
	}
}

void asteroid_control()
{
	void break_asteroid(struct asteroid *obj)
	{
		if (obj->size==2)
		{
			ast[ast_c].size=1;
			ast[ast_c].x=obj->x+5;
			ast[ast_c].y=obj->y;
			ast[ast_c].xsp=obj->xsp+2;
			ast[ast_c].ysp=obj->ysp;
			ast_c=(ast_c+1)%ASTEROID_MAX;
			ast[ast_c].size=1;
			ast[ast_c].x=obj->x-5;
			ast[ast_c].y=obj->y;
			ast[ast_c].xsp=obj->xsp-2;
			ast[ast_c].ysp=obj->ysp;
			ast_c=(ast_c+1)%ASTEROID_MAX;
		}
		obj->size+=-1;

		eff[eff_c].life=10;
		eff[eff_c].x=obj->x;
		eff[eff_c].y=obj->y;
		eff[eff_c].r=0xff;
		eff[eff_c].g=0xaa;
		eff[eff_c].b=0x11;
		eff[eff_c].radius=0;
		eff[eff_c].draw=&draw_effect_explosion;
		eff[eff_c].update=&update_effect_explosion;

		eff_c=(eff_c+1)%EFFECT_MAX;
	}

	int num;	//number of asteroids remaining

	num=0;
	int i;
	for (i=0;i<ASTEROID_MAX;i++)
	{
		if (ast[i].size==0)
			continue;

		draw_asteroid(&ast[i]);
		update_asteroid(&ast[i]);

		//Collision detection
		int j;
		for (j=0;j<MISSILE_MAX;j++)
			if (missl[j].life>0 && asteroid_x_missile(&ast[i],&missl[j]))
			{
				missl[j].life=0;

				ast[i].xsp=missl[j].xsp;
				ast[i].ysp=missl[j].ysp;
				break_asteroid(&ast[i]);
				game.kills+=1;
				game.total+=1;
			}
		if (asteroid_x_ship(&ast[i],&user))
		{
			user.health+=-1;
			user.xsp=ast[i].xsp;
			user.ysp=ast[i].ysp;

			break_asteroid(&ast[i]);
		}

		num+=ast[i].size;
	}

	if (num==0)
	{
		game.end_flags=game.end_flags|GAME_AST_FLAG;
	}
}

void target_control()
{
	int i;
	for (i=0;i<10;i++)
	{
		draw_target(&tgt[i]);

		int j;
		for (j=0;j<MISSILE_MAX;j++)
			if (missl[j].life>0 && target_x_missile(&tgt[i],&missl[j]))
			{
				missl[j].life=0;
				tgt[i].alive+=-1;

				eff[eff_c].life=10;
				eff[eff_c].x=tgt[i].x;
				eff[eff_c].y=tgt[i].y;
				eff[eff_c].radius=0;
				eff[eff_c].r=0xff;
				eff[eff_c].g=0xaa;
				eff[eff_c].b=0x11;
				eff[eff_c].draw=&draw_effect_explosion;
				eff[eff_c].update=&update_effect_explosion;

				eff_c=(eff_c+1)%EFFECT_MAX;

				game.kills+=1;
				game.total+=1;
			}
	}
}

void combo_control()
{
	int i;

	if (game.end_flags==(game.flags & (~GAME_ALL_FLAG)))
	{
		int type=(xor128()+xor128())%(game.wave>4?7:3);
		switch (type)
		{
			case 0:
				game.flags=GAME_ALL_FLAG|GAME_TNK_FLAG;
				for (i=0;i<TANK_MAX;i++)
					tnk[i].health=0;
				break;
			case 1:
				game.flags=GAME_ALL_FLAG|GAME_AIR_FLAG;
				for (i=0;i<BALLOON_MAX;i++)
				{
					air[i].gun_health=0;
					air[i].float_health=0;
				}
				break;
			case 3:
				game.flags=GAME_ALL_FLAG|GAME_AST_FLAG;
				for (i=0;i<ASTEROID_MAX;i++)
					ast[i].size=0;
				break;
			case 2:
				game.flags=GAME_ALL_FLAG|GAME_TUR_FLAG|GAME_MSL_FLAG;
				game.kills=0;
				for (i=0;i<SCUD_MAX;i++)
					scd[i].life=0;
				for (i=0;i<TURRET_MAX;i++)
					trt[i].health=0;
				break;
			case 4:
				game.flags=GAME_ALL_FLAG|GAME_AIR_FLAG|GAME_TNK_FLAG;
				for (i=0;i<BALLOON_MAX;i++)
				{
					air[i].gun_health=0;
					air[i].float_health=0;
					tnk[i].health=0;
				}
				break;
			case 5:
				game.flags=GAME_ALL_FLAG|GAME_AIR_FLAG|GAME_TUR_FLAG;
				for (i=0;i<BALLOON_MAX;i++)
				{
					air[i].gun_health=0;
					air[i].float_health=0;
				}
				for (i=0;i<TURRET_MAX;i++)
					trt[i].health=0;
				break;
			case 6:
				game.flags=GAME_ALL_FLAG|GAME_AST_FLAG|GAME_TUR_FLAG;
				for (i=0;i<ASTEROID_MAX;i++)
					ast[i].size=0;
				for (i=0;i<TURRET_MAX;i++)
					trt[i].health=0;
				break;
		}
		if (game.wave>3)
			user.health+=1; //Make it a little more "fair"
		game.end_flags=game.flags; //Artificially trigger next wave
	}
}

void setup_game()
{
	int i;

	user.x=vinfo.xres/3;
	user.y=vinfo.yres-50;
	user.xsp=0;
	user.ysp=0;
	user.rot=M_PI*0.5;
	user.reload=20;
	user.arm=20;
	user.range=120;
	user.health=5;
	user.loaded=5;

	game.wave=0;
	game.kills=0;
	game.total=0;
	game.end_flags=0;
	game.end_timer=100;

	if (game.flags&GAME_AST_FLAG)
		for (i=0;i<ASTEROID_MAX;i++)
			ast[i].size=0;
	if (game.flags&GAME_TGT_FLAG)
	{
		tgt[0].x=vinfo.xres/2;
		tgt[0].y=vinfo.yres/2;
		tgt[0].alive=1;

		tgt[1].x=vinfo.xres/2;
		tgt[1].y=vinfo.yres/2-150;
		tgt[1].alive=1;

		tgt[2].x=vinfo.xres/2;
		tgt[2].y=vinfo.yres/2-300;
		tgt[2].alive=1;

		tgt[3].x=vinfo.xres/2;
		tgt[3].y=vinfo.yres/2+300;
		tgt[3].alive=1;

		tgt[4].x=vinfo.xres/2-150;
		tgt[4].y=vinfo.yres/2-150;
		tgt[4].alive=1;

		tgt[5].x=vinfo.xres/2-150;
		tgt[5].y=vinfo.yres/2-300;
		tgt[5].alive=1;

		tgt[6].x=vinfo.xres/2-300;
		tgt[6].y=vinfo.yres/2-300;
		tgt[6].alive=1;

		tgt[7].x=vinfo.xres/2+150;
		tgt[7].y=vinfo.yres/2-150;
		tgt[7].alive=1;

		tgt[8].x=vinfo.xres/2+150;
		tgt[8].y=vinfo.yres/2-300;
		tgt[8].alive=1;

		tgt[9].x=vinfo.xres/2+300;
		tgt[9].y=vinfo.yres/2-300;
		tgt[9].alive=1;
	}
	if (game.flags&GAME_TUR_FLAG)
	{
		trt[0].x=vinfo.xres/2;
		trt[0].y=750;
		trt[0].health=1;
		trt[0].loaded=10;
	}
	game.end_flags=game.flags & (~GAME_ALL_FLAG);
	game_condition();
}

void game_loop()
{
	int i;

	clear();

	draw_hud();
	draw_user_health();

	draw_ship(&user);
	update_ship(&user);

	if (game.flags&GAME_AST_FLAG)
		asteroid_control();
	if (game.flags&GAME_MSL_FLAG)
		scud_control();
	if (game.flags&GAME_TUR_FLAG)
		turret_control();
	if (game.flags&GAME_TNK_FLAG)
		tank_control();
	if (game.flags&GAME_TGT_FLAG)
		target_control();
	if (game.flags&GAME_AIR_FLAG)
		balloon_control();
	if (game.flags&GAME_ALL_FLAG)
		combo_control();

	//Update missiles
	for (i=0;i<MISSILE_MAX;i++)
	{
		if (missl[i].life<=0)
			continue;

		draw_missile(&missl[i]);
		update_missile(&missl[i]);

		if (ship_x_missile(&user,&missl[i]))
		{
			missl[i].life=0;
			user.health+=-1;

			eff[eff_c].life=5;
			eff[eff_c].radius=5;
			eff[eff_c].x=missl[i].x-missl[i].xsp;
			eff[eff_c].y=missl[i].y-missl[i].ysp;
			eff[eff_c].r=0xff;
			eff[eff_c].g=0xaa;
			eff[eff_c].b=0x11;
			eff[eff_c].draw=&draw_effect_explosion;
			eff[eff_c].update=&update_effect_explosion;

			eff_c=(eff_c+1)%EFFECT_MAX;
		}
	}

	for (i=0;i<EFFECT_MAX;i++)
	{
		if (eff[i].life<=0)
			continue;
		
		eff[i].draw(&eff[i]);
		eff[i].update(&eff[i]);
	}

	game_condition();

	if (key[KEY_LEFT])
		user.rot+=0.094;
	if (key[KEY_RIGHT])
		user.rot+=-0.094;
	if (key[KEY_UP])
	{
		user.xsp+=cos(user.rot);
		user.ysp+=-sin(user.rot);
	}
	if (key[KEY_SPACE])
	{
		if (user.loaded<=0)
		{
			missl[msl_c].xsp=(user.xsp)+(cos(user.rot)*10);
			missl[msl_c].ysp=(user.ysp)-(sin(user.rot)*10);
			missl[msl_c].x=user.x+(user.xsp)+(cos(user.rot)*30);
			missl[msl_c].y=user.y+(user.ysp)-(sin(user.rot)*30);
			missl[msl_c].arm=user.arm;
			missl[msl_c].life=user.range;
			user.loaded=user.reload;

			msl_c=(msl_c+1)%MISSILE_MAX;
		}
	}
	if (key[KEY_ESC])
	{
		clear();
		loop=menu_loop;
	}

	swap_buffers();

	//Sleep for awhile
	struct timespec t = {0,40000000};
	syscall2(SYS_nanosleep,&t, NULL);
}

void menu_loop()
{
	static uint8_t selection=0;
	static signed int fade=0;
	static double rot=M_PI*0.5;

	if (key[KEY_DOWN] || key[KEY_RIGHT])
		fade+=1;
	else if (key[KEY_UP] || key[KEY_LEFT])
		fade+=-1;
	else if (fade>0)
		fade+=-1;
	else if (fade<0)
		fade+=1;

	uint32_t upcol=(key[KEY_UP] || key[KEY_LEFT])?0x00FF5555:0x00770000;
	uint32_t dncol=(key[KEY_DOWN] || key[KEY_RIGHT])?0x00FF5555:0x00770000;
	int x,y;
	x=vinfo.xres/2+100;
	y=vinfo.yres/2;
	draw_line_fast(x,y+10,x,y+100,dncol);
	draw_line_fast(x,y+100,x+25,y+75,dncol);
	draw_line_fast(x,y+100,x-25,y+75,dncol);
	draw_line_fast(x,y-10,x,y-100,upcol);
	draw_line_fast(x,y-100,x+25,y-75,upcol);
	draw_line_fast(x,y-100,x-25,y-75,upcol);

	if (fade>10)
	{
		fade=-10;
		if (selection==6) selection=0;
		else selection+=1;
	}
	if (fade<-10)
	{
		fade=10;
		if (selection==0) selection=6;
		else selection+=-1;
	}

	if (selection==6)
	{
		draw_exit_menu(10-ABS(fade));
	}
	if (selection==0)
	{
		struct target menu_target;
		menu_target.x=vinfo.xres/2;
		menu_target.y=vinfo.yres/2;
		draw_target_menu(&menu_target,10-ABS(fade));
	}
	if (selection==1)
	{
		struct asteroid menu_asteroid;
		menu_asteroid.x=vinfo.xres/2;
		menu_asteroid.y=vinfo.yres/2;
		draw_asteroid_menu(&menu_asteroid,10-ABS(fade));
	}
	if (selection==2)
	{
		struct turret menu_turret;
		menu_turret.x=vinfo.xres/2;
		menu_turret.y=vinfo.yres/2;
		menu_turret.rot=rot;
		draw_turret_menu(&menu_turret,10-ABS(fade));
	}
	if (selection==3)
	{
		struct tank menu_tank;
		menu_tank.x=vinfo.xres/2;
		menu_tank.y=vinfo.yres/2;
		menu_tank.rot=rot;
		draw_tank_menu(&menu_tank,10-ABS(fade));
	}
	if (selection==4)
	{
		struct balloon menu_balloon;
		menu_balloon.x=vinfo.xres/2;
		menu_balloon.y=vinfo.yres/2+40;
		menu_balloon.rot=-rot;
		draw_balloon_menu(&menu_balloon,10-ABS(fade));
	}
	if (selection==5)
	{
		struct balloon menu_balloon;
		menu_balloon.x=vinfo.xres/2+50;
		menu_balloon.y=vinfo.yres/2+40;
		menu_balloon.rot=-rot;
		draw_balloon_menu(&menu_balloon,10-ABS(fade));

		struct tank menu_tank;
		menu_tank.x=vinfo.xres/2-50;
		menu_tank.y=vinfo.yres/2+50;
		menu_tank.rot=rot;
		draw_tank_menu(&menu_tank,10-ABS(fade));

		struct turret menu_turret;
		menu_turret.x=vinfo.xres/2;
		menu_turret.y=vinfo.yres/2+50;
		menu_turret.rot=rot;
		draw_turret_menu(&menu_turret,10-ABS(fade));

		struct asteroid menu_asteroid;
		menu_asteroid.x=vinfo.xres/2;
		menu_asteroid.y=vinfo.yres/2-50;
		draw_asteroid_menu(&menu_asteroid,10-ABS(fade));

		struct target menu_target;
		menu_target.x=vinfo.xres/2;
		menu_target.y=vinfo.yres/2;
		draw_target_menu(&menu_target,10-ABS(fade));
	}

	if (key[KEY_SPACE] || key[KEY_ENTER])
	{
		loop=game_loop;
		switch (selection)
		{
			case 0:
				game.flags=GAME_TGT_FLAG;
				break;
			case 1:
				game.flags=GAME_AST_FLAG;
				break;
			case 2:
				game.flags=GAME_TUR_FLAG | GAME_MSL_FLAG;
				break;
			case 3:
				game.flags=GAME_TNK_FLAG;
				break;
			case 4:
				game.flags=GAME_AIR_FLAG;
				break;
			case 5:
				game.flags=GAME_ALL_FLAG;
				break;
			default:
				cleanup(0);
				break;
		}
		setup_game();
	}

	swap_buffers();

//		uint16_t tic=gettime();
	//Sleep for awhile
	struct timespec t = {0,50000000};
	syscall2(SYS_nanosleep,&t, NULL);
//		tic=gettime();
//		writeint(0,tic);
//		syscall3(SYS_write,0," MENU \n",7);
}

void end_loop()
{
	draw_end_hud();

	swap_buffers();

	if (key[KEY_SPACE] || key[KEY_ENTER])
	{
		clear();
		setup_game();
		loop=game_loop;
	}
	if (key[KEY_BACKSPACE] || key[KEY_ESC])
	{
		clear();
		loop=menu_loop;
	}

	struct timespec t = {0,50000000};
	syscall2(SYS_nanosleep,&t, NULL);
}

int main(int argc, char *argv[])
{
	int fbfd = 0;
	int evfd = 0;
	long int screensize = 0;
	int x = 0, y = 0, i = 0;
	uint8_t pixel = 0;
	long int location = 0;

	//Open keyboard
	if (argc==1)
		evfd = syscall2(SYS_open,"/dev/input/event5", O_RDONLY|O_NONBLOCK);
	else
		evfd = syscall2(SYS_open,argv[1], O_RDONLY|O_NONBLOCK);

	//Open framebuffer
	fbfd = syscall2(SYS_open,"/dev/fb0", O_RDWR);

	if (fbfd==0)
		syscall3(SYS_write,0,"Framebuffer opening error\n",26);

	//Get variable screen information
	syscall3(SYS_ioctl,fbfd, FBIOGET_VSCREENINFO, &vinfo);
	vinfo.bits_per_pixel=32;
	syscall3(SYS_ioctl,fbfd, FBIOPUT_VSCREENINFO, &vinfo);
	syscall3(SYS_ioctl,fbfd, FBIOGET_VSCREENINFO, &vinfo);

	//Get fixed screen information
	syscall3(SYS_ioctl,fbfd, FBIOGET_FSCREENINFO, &finfo);

	//Figure out the size of the screen in bytes
	screensize = vinfo.yres_virtual * finfo.line_length;

	//Map the framebuffer to memory
	long syscall_memblock[] = {(long)0, (long) screensize,(long) (PROT_READ | PROT_WRITE), (long)MAP_SHARED, (long)fbfd, (off_t)0};
	fbp = (uint8_t *)syscall_list(SYS_mmap, syscall_memblock);

	if ((long)fbp==-1)
		syscall3(SYS_write,0,"Mapping Error\n",14);

	//Initialize the back buffer
	init_double_buffer();

	if ((long)back_buffer==-1)
		syscall3(SYS_write,0,"Back Buffer Error\n",18);

	//Save the previous contents of the framebuffer so we can play on top of 
	//the old screen instead of learing it
//	uint32_t buffer[vinfo.xres*vinfo.yres];
//	syscall3(SYS_read,fbfd,buffer,vinfo.xres*vinfo.yres*4);

	//Start with the menu
	loop=menu_loop;

	gettime();

	//Main game loop
	while (1)
	{
		while (get_keys(evfd)) ;

		loop();
	}

	syscall2(SYS_munmap,fbp, screensize);
	syscall1(SYS_close,fbfd);

	return 0;
}
