#define MOD_NAME "Building Drone"
#define MOONLAB_LATE_UPDATE_ROLE 2
#include "common.h"
#include "../../runtime/native/camera.h"
#include "../../runtime/native/input.h"
#include "storage.h"
#include "../shop-framework/client.h"
#include "../equipment-framework/sdk/client.h"
#include "../hints-framework/sdk/client.h"
static const MoonShop *fw_shop;
static const MoonEquipment *fw_equipment;
static const MoonHints *fw_hints;
static const MoonCameraApiV1 *fw_camera;
static MoonCameraSession camera_session;
static int fw_started;
static struct {
 PTR boots,mounted,climbing,resting,flying,kinematic,set_kinematic,velocity,set_velocity;
 DRoot camera,body;DVec home,camera_home,position;Quat rotation;int active,returning,owned,old_kinematic,price;float speed,range;
 char world_key[32];const char *status;
} drone;
static int alive(PTR o){int ok=1;void *a[]={o};PTR v=o?call(sx.alive,0,a,&ok):0;return ok&&v&&*(unsigned char*)unbox(v);}
#include "camera_restore.h"
static int capture_camera(void *context){
 PTR walker=context;int ok=1;PTR camera=cf(walker,"viewCamera"),body=cf(walker,"body");
 if(!alive(camera)||!alive(body))return 0;
 drone.camera=dhold(camera);drone.body=dhold(body);
 if(!drone.camera||!drone.body){ddrop(&drone.camera);ddrop(&drone.body);return 0;}
 PTR tr=ctr(camera,&ok);drone.position=drone.camera_home=dvector(cm.position,tr,&ok);
 PTR q=call(cm.rotation,tr,0,&ok);if(q)dcopy(&drone.rotation,unbox(q),sizeof(Quat));else ok=0;
 drone.home=dvector(cm.position,ctr(walker,&ok),&ok);drone.old_kinematic=boolean(drone.kinematic,body,&ok);
 if(!ok){ddrop(&drone.camera);ddrop(&drone.body);}return ok;
}
static int gate(PTR walker){int ok=1;return boolean(drone.boots,walker,&ok)&&ok;}
static Quat multiply(Quat a,Quat b){return (Quat){a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
static Quat turn(float pitch,float yaw){
 /* Per-frame angles are bounded by the 0.2 s step gate. Taylor terms retain
  * the axis-angle result without binding Unity's stripped Rotate overload. */
 float x=pitch*.00872664626f,y=yaw*.00872664626f,xx=x*x,yy=y*y;
 Quat qx={x*(1-xx/6+xx*xx/120),0,0,1-xx/2+xx*xx/24-xx*xx*xx/720};
 Quat qy={0,y*(1-yy/6+yy*yy/120),0,1-yy/2+yy*yy/24-yy*yy*yy/720};return multiply(qy,qx);
}
static void buy(PTR walker){if(drone.owned)return;if(!gate(walker)){drone.status="Requires vanilla Gravity Boots upgrade";return;}if(!drone.world_key[0]){drone.status="World identity unavailable; purchase disabled";return;}if(!ccharge(drone.price)){drone.status="Insufficient funds or purchase unavailable";return;}drone.owned=1;if(!io.ini_write("owned-worlds",drone.world_key,"1",sx.config))drone.status="Purchased this session; ownership save failed";else drone.status="Drone purchased; Alt+D deploys";sx_event("drone-purchased",drone.price);}
static void deploy(PTR walker){
 int ok=1;if(!drone.owned){drone.status="Buy the drone from the equipment shop";return;}
 if(boolean(drone.mounted,walker,&ok)||boolean(drone.climbing,walker,&ok)||boolean(drone.resting,walker,&ok)||boolean(drone.flying,walker,&ok)||!ok){drone.status="Stand on foot with player flight off before deploying";return;}
 PTR camera=cf(walker,"viewCamera");
 if(!moon_camera_begin(&camera_session,fw_camera,"building-drone",dget(cm.world),walker,camera,MOON_CAMERA_ALL,capture_camera,walker)){drone.status="Camera unavailable or used by another view";return;}
 drone.active=1;unsigned char yes=1;cv(drone.set_kinematic,dget(drone.body),&yes,&ok);
 if(!ok){stop();return;}drone.status="Remote build camera active";sx_event("drone-deployed",0);
}
static void camera_step(void){int ok=1;PTR walker=dget(cm.walker);if(drone.returning||!cinput(walker)||!drone.active){stop();return;}PTR camera=dget(drone.camera);if(!alive(camera)||camera!=cf(walker,"viewCamera")){stop();return;}PTR tr=ctr(camera,&ok);float dt=dnumber(cm.delta,0,&ok);if(!ok||dt<=0||dt>.2f)return;
 PTR boxed=call(cm.rotation,tr,0,&ok);if(!ok||!boxed){stop();return;}Quat current;dcopy(&current,unbox(boxed),sizeof(current));
 Quat next_rotation=multiply(current,turn((float)(cm.down[0x28]-cm.down[0x26])*75*dt,(float)(cm.down[0x27]-cm.down[0x25])*75*dt));
 float magnitude=croot(next_rotation.x*next_rotation.x+next_rotation.y*next_rotation.y+next_rotation.z*next_rotation.z+next_rotation.w*next_rotation.w);
 if(magnitude<.5f){stop();return;}next_rotation.x/=magnitude;next_rotation.y/=magnitude;next_rotation.z/=magnitude;next_rotation.w/=magnitude;cv(cm.set_rotation,tr,&next_rotation,&ok);
 DVec f=dvector(cm.forward,tr,&ok),r=dvector(cm.right,tr,&ok),up=unit(drone.home);DVec delta=ca(cx(f,(float)(cm.down['W']-cm.down['S'])),ca(cx(r,(float)(cm.down['D']-cm.down['A'])),cx(up,(float)(cm.down[0x20]-cm.down[0x11]))));float length=croot(cd(delta,delta));if(length>1)delta=cx(delta,1/length);DVec next=ca(drone.position,cx(delta,drone.speed*dt*(cm.down[0x10]?2:1)));DVec from_home=ca(next,cx(drone.home,-1));float distance=croot(cd(from_home,from_home));if(distance>drone.range)next=ca(drone.home,cx(from_home,drone.range/distance));drone.position=next;cv(cm.set_position,tr,&drone.position,&ok);if(!ok)stop();}
static int fw_context(PTR world,PTR walker){int ok=1;return world==dget(cm.world)&&walker==dget(cm.walker)&&sx_world(walker,&ok)==world&&ok;}
static unsigned fw_state(PTR world,PTR walker){
 if(!fw_context(world,walker))return 0;int ok=1;unsigned state=MS_AVAILABLE|(drone.owned?MS_OWNED:0);
 if(!sx_host(&ok)||!ok||!drone.world_key[0]||!gate(walker))state|=MS_BLOCKED;return state;
}
static int fw_purchase(PTR world,PTR walker){
 if(fw_state(world,walker)!=MS_AVAILABLE)return 0;int ok=1;
 if(!boolean(sx.focused,0,&ok)||boolean(sx.typing,0,&ok)||!ok)return 0;
 buy(walker);return drone.owned?1:0;
}
static int fw_owned(PTR world,PTR walker){return fw_context(world,walker)&&drone.owned;}
static int fw_bind(PTR domain,PTR game){
 fw_shop=ms_connect(domain,game);fw_equipment=me_connect(domain,game);fw_hints=mh_connect(domain,game);if(!fw_shop||!fw_equipment||!fw_hints)return 0;
 MoonShopDefinition offer={MS_ABI,sizeof(offer),"building-drone","Building Drone","Remote building camera. Requires the Gravity Boots upgrade.",drone.price,fw_state,fw_purchase};
 MoonEquipmentDefinition item={ME_ABI,sizeof(item),"building-drone","Building Drone","A controller for the remote building camera.",0,fw_owned,0};
 return fw_shop->register_item(&offer)&&fw_equipment->register_item(&item);
}
static int before(int id,Context *c){PTR walker=(PTR)c->rcx;if(id){if(drone.active&&walker==dget(cm.walker)&&sx_thread()){if(id==1)camera_step();return drone.active;}return 0;}
 if(!sx_thread())return 0;if(drone.returning){stop();if(camera_session.token)return 0;}
 int state=cframe(walker);MoonInputState deploy_action=moon_input_action("building-drone.deploy","!Ctrl+Alt+D",state>0&&cinput(walker));if(state<0){stop();if(camera_session.token)return 0;ddrop(&cm.world);ddrop(&cm.walker);int ok=1;cm.world=dhold(sx_world(walker,&ok));cm.walker=dhold(walker);drone.world_key[0]=0;drone.owned=0;if(dget(cm.world)&&worldkey(drone.world_key))drone.owned=(int)sx.ini_int("owned-worlds",drone.world_key,0,sx.config)==1;drone.status="";return 0;}if(state<=0){stop();return 0;}if(!cinput(walker)){if(drone.active)stop();return 0;}
 int equipped=fw_equipment->is_equipped("building-drone",walker);
 if(deploy_action.pressed){if(drone.active)stop();else if(equipped)deploy(walker);}
 if(drone.active&&cm.press[0x1b])stop();
 if(drone.active)fw_hints->publish("building-drone",walker,"Alt+D: Return | WASD: Fly | Space/Ctrl: Rise/descend | Arrows: Look | Shift: Faster | Esc: Return");
 else if(equipped)fw_hints->publish("building-drone",walker,"Alt+D: Deploy building drone");
 else fw_hints->clear("building-drone");return 0;}
__declspec(dllexport) int MoonLab_ModStart(PTR domain,PTR game){if(fw_started)return 1;if(!sx_start(domain,game,"building-drone","MOONLAB_BUILDING_DRONE_CONFIG")||!cbind(game)||!iobind("building-drone"))return 0;
#define D(F,N,T,M,S) drone.F=dmethod(N,T,M,S);if(!drone.F){sx_event(T "::" M,0);return 0;}
 D(boots,"MoonCleaners","MoonWalker","get_OwnsGravityBoots","");D(mounted,"MoonCleaners","MoonWalker","get_Mounted","");D(climbing,"MoonCleaners","MoonWalker","get_Climbing","");D(resting,"MoonCleaners","MoonWalker","get_Resting","");D(flying,"MoonCleaners","MoonWalker","get_Flying","");D(kinematic,"UnityEngine","Rigidbody","get_isKinematic","");D(set_kinematic,"UnityEngine","Rigidbody","set_isKinematic","System.Boolean");
#undef D
 drone.price=sx_limit("drone","price",2000,1,1000000);drone.speed=(float)sx_limit("drone","speed",12,1,60);drone.range=(float)sx_limit("drone","range",80,10,300);(void)ci;(void)cn;(void)cs;(void)craycast;(void)iowrite;(void)ioread;(void)valid;(void)dzero;(void)dtype;(void)darray;
 if(!prepare_hook(method("MoonCleaners","MoonWalker","Update",0),100671733,0,game)||!prepare_hook(method("MoonCleaners","MoonWalker","LateUpdate",0),100671741,1,game)||!prepare_hook(method("MoonCleaners","MoonWalker","FixedUpdate",0),100671734,2,game))return 0;
 fw_camera=moon_camera_connect();if(!fw_camera||!fw_bind(domain,game))return 0;before_hook=before;if(!install_hooks())return 0;fw_started=1;sx_event("ready-testing-deferred",0);return 1;}
