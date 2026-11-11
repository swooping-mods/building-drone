/* Restore the captured targets before releasing the shared camera lease. */
static int restore_camera(void *context){
 (void)context;int ok=1;PTR body=dget(drone.body),camera=dget(drone.camera);
 if(moon_camera_target_alive(sx.alive,camera,&ok)){PTR tr=ctr(camera,&ok);cv(cm.set_position,tr,&drone.camera_home,&ok);cv(cm.set_rotation,tr,&drone.rotation,&ok);}
 if(moon_camera_target_alive(sx.alive,body,&ok)){unsigned char old=(unsigned char)drone.old_kinematic;cv(drone.set_kinematic,body,&old,&ok);}
 return ok;
}
static void stop(void){
 drone.returning=1;
 if(!moon_camera_finish(&camera_session,restore_camera,0))return;
 if(drone.active)sx_event("drone-returned",0);
 drone.active=drone.returning=0;ddrop(&drone.camera);ddrop(&drone.body);if(fw_hints)fw_hints->clear("building-drone");
}
