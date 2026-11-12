/* Retained cm identities may change only after the original snapshot restores.
 * Local admission precedes cframe: remote callbacks cannot spend its frame latch. */
static int drone_original_context_alive(PTR walker,int *ok){
 PTR world=dget(cm.world);
 return walker&&walker==dget(cm.walker)&&world&&
        moon_camera_target_alive(sx.alive,world,ok)&&
        moon_camera_target_alive(sx.alive,walker,ok)&&*ok;
}
static int drone_context_current(PTR walker){
 int ok=1;
 if(!drone_original_context_alive(walker,&ok)||!moon_local_actor_admit(walker)||
    sx_world(walker,&ok)!=dget(cm.world)||!ok||
    cf(walker,"viewCamera")!=dget(drone.camera)||cf(walker,"body")!=dget(drone.body))return 0;
 return moon_camera_target_alive(sx.alive,dget(drone.camera),&ok)&&
        moon_camera_target_alive(sx.alive,dget(drone.body),&ok)&&ok;
}
static int drone_update_admit(PTR walker){
 if(!sx_thread())return 0;
 PTR local=0;
 if(!moon_local_actor_read(&local)){if(drone.active||drone.returning)stop();return 0;}
 if(drone.returning||((drone.active||camera_session.token)&&local!=dget(cm.walker))){
  stop();if(camera_session.token)return 0;
 }
 return local&&walker==local;
}
static int drone_owned_hook(int id,PTR walker){
 if(!sx_thread()||!drone.active||walker!=dget(cm.walker))return 0;
 if(drone.returning||!drone_context_current(walker))stop();
 else if(id==1)camera_step();
 /* While cleanup retries, keep the still-parked originals suppressed even
  * after local-owner/current-world loss. This never authorizes new flight.
  * A destroyed/uncertain original world or actor forbids field reads; a
  * replacement camera/body keeps its own vanilla callback. */
 int ok=1;
 return drone.active&&drone_original_context_alive(walker,&ok)&&
        (id==1?cf(walker,"viewCamera")==dget(drone.camera):
               cf(walker,"body")==dget(drone.body));
}
