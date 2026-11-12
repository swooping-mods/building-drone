/* Failed/missing native boxes cannot become a captured restoration snapshot. */
static void drone_read(PTR method,PTR self,void *out,U64 size,int *ok){
 if(!*ok)return;
 PTR boxed=call(method,self,0,ok);if(!*ok||!boxed){*ok=0;return;}
 PTR value=unbox(boxed);if(!value){*ok=0;return;}dcopy(out,value,size);
}
static int capture_camera(void *context){
 PTR walker=context;int ok=1;PTR camera=cf(walker,"viewCamera"),body=cf(walker,"body");
 if(!moon_camera_target_alive(sx.alive,camera,&ok)||!moon_camera_target_alive(sx.alive,body,&ok)||!ok)return 0;
 drone.camera=dhold(camera);drone.body=dhold(body);
 if(!drone.camera||!drone.body){ddrop(&drone.camera);ddrop(&drone.body);return 0;}
 PTR tr=ctr(camera,&ok);if(!tr)ok=0;
 drone_read(cm.position,tr,&drone.camera_home,sizeof(drone.camera_home),&ok);drone.position=drone.camera_home;
 drone_read(cm.rotation,tr,&drone.rotation,sizeof(drone.rotation),&ok);
 tr=ctr(walker,&ok);if(!tr)ok=0;
 drone_read(cm.position,tr,&drone.home,sizeof(drone.home),&ok);
 unsigned char kinematic=0;drone_read(drone.kinematic,body,&kinematic,sizeof(kinematic),&ok);drone.old_kinematic=kinematic!=0;
 if(!valid(drone.camera_home)||!valid(drone.home)||!finite(drone.rotation.x)||!finite(drone.rotation.y)||!finite(drone.rotation.z)||!finite(drone.rotation.w))ok=0;
 if(!ok){ddrop(&drone.camera);ddrop(&drone.body);}return ok;
}
