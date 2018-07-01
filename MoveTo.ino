// -----------------------------------------------------------------------------------
// Functions to move the mount to the a new position

// moves the mount
void moveTo() {
  // HA goes from +90...0..-90
  //                W   .   E
  // meridian flip, first phase.  only happens for GEM mounts
  if ((pierSideControl==PierSideFlipEW1) || (pierSideControl==PierSideFlipWE1)) {

    // save destination
    cli(); 
    origTargetAxis1.fixed = targetAxis1.fixed;
    origTargetAxis2 = (long)targetAxis2.part.m;
 
    timerRateAxis1=SiderealRate;
    timerRateAxis2=SiderealRate;
    sei();

    // first phase, decide if we should move to 60 deg. HA (4 hours) to get away from the horizon limits or just go straight to the home position
    if (pierSideControl==PierSideFlipWE1) {
      if (celestialPoleAxis1==0.0) setTargetAxis1(0.0,PierSideWest); else {
        if ((currentAlt<10.0) && (getStartAxis1()>-90.0)) setTargetAxis1(-60.0,PierSideWest); else setTargetAxis1(-celestialPoleAxis1,PierSideWest);
      }
      setTargetAxis2(celestialPoleAxis2,PierSideWest);
    } else {
      if (celestialPoleAxis1==0.0) setTargetAxis1(0.0,PierSideEast); else {
        if ((currentAlt<10.0) && (getStartAxis1()<90.0)) setTargetAxis1(60.0,PierSideEast); else setTargetAxis1(celestialPoleAxis1,PierSideEast);
      }
      setTargetAxis2(celestialPoleAxis2,PierSideEast);
    }

    // first phase, override above for additional waypoints
    if (celestialPoleAxis2>0.0) {
      if (getInstrAxis2()>90.0-latitude) {
        // if Dec is in the general area of the pole, slew both axis back at once
        if (pierSideControl==PierSideFlipWE1) setTargetAxis1(-celestialPoleAxis1,PierSideWest); else setTargetAxis1(celestialPoleAxis1,PierSideEast);
      } else {
        // if we're at a low latitude and in the opposite sky, |HA|=6 is very low on the horizon in this orientation and we need to delay arriving there during a meridian flip
        // in the extreme case, where the user is very near the (Earths!) equator an Horizon limit of -10 or -15 may be necessary for proper operation.
        if ((currentAlt<20.0) && (abs(latitude)<45.0) && (getInstrAxis2()<0.0)) {
          if (pierSideControl==PierSideFlipWE1) setTargetAxis1(-45.0,PierSideWest); else setTargetAxis1(45.0,PierSideEast);
        }
      }
    } else {
      if (getInstrAxis2()<-90.0-latitude) {
        // if Dec is in the general area of the pole, slew both axis back at once
        if (pierSideControl==PierSideFlipWE1) setTargetAxis1(-celestialPoleAxis1,PierSideWest); else setTargetAxis1(celestialPoleAxis1,PierSideEast);
      } else { 
        // if we're at a low latitude and in the opposite sky, |HA|=6 is very low on the horizon in this orientation and we need to delay arriving there during a meridian flip
        if ((currentAlt<20.0) && (abs(latitude)<45.0) && (getInstrAxis2()>0.0)) {
          if (pierSideControl==PierSideFlipWE1) setTargetAxis1(-45.0,PierSideWest); else setTargetAxis1(45.0,PierSideEast);
        }
      }
    }

    D("Flp1 Axis1, Current "); D(((double)(long)posAxis1)/(double)StepsPerDegreeAxis1); D(" -to-> "); DL(((double)(long)targetAxis1.part.m)/(double)StepsPerDegreeAxis1);
    D("Flp1 Axis2, Current "); D(((double)(long)posAxis2)/(double)StepsPerDegreeAxis2); D(" -to-> "); DL(((double)(long)targetAxis2.part.m)/(double)StepsPerDegreeAxis2); DL("");
    pierSideControl++;
    forceRefreshGetEqu();
  }

  long distStartAxis1,distStartAxis2,distDestAxis1,distDestAxis2;

  cli();
  distStartAxis1=abs(posAxis1-startAxis1);  // distance from start Axis1
  distStartAxis2=abs(posAxis2-startAxis2);  // distance from start Axis2
  sei();
  if (distStartAxis1<1) distStartAxis1=1;
  if (distStartAxis2<1) distStartAxis2=1;

  Again:
  cli();
  distDestAxis1=abs(posAxis1-(long)targetAxis1.part.m);  // distance from dest Axis1
  distDestAxis2=abs(posAxis2-(long)targetAxis2.part.m);  // distance from dest Axis2
  sei();
  
  long a2=abs((getInstrAxis2()-getTargetAxis2())*StepsPerDegreeAxis2);
  static long lastPosAxis2=0;

  // adjust rates near the horizon to help keep from exceeding the minAlt limit
  #ifndef MOUNT_TYPE_ALTAZM
  if (latitude<0) a2=-a2;
  if (((latitude>10) || (latitude<-10)) && (distStartAxis1>((DegreesForAcceleration*StepsPerDegreeAxis1)/16))) {
    // if Dec is decreasing, slow down Dec
    if (a2<lastPosAxis2) {
      double minAlt2=minAlt+10.0;
      long a=(currentAlt-minAlt2)*StepsPerDegreeAxis2; if (a<((DegreesForAcceleration*StepsPerDegreeAxis2)/8)) a=((DegreesForAcceleration*StepsPerDegreeAxis2)/8);
      if (a<distDestAxis2) distDestAxis2=a;
    } else
    // if Dec is increasing, slow down HA
    {
      double minAlt2=minAlt+10.0;
      long a=(currentAlt-minAlt2)*StepsPerDegreeAxis1; if (a<((DegreesForAcceleration*StepsPerDegreeAxis1)/8)) a=((DegreesForAcceleration*StepsPerDegreeAxis1)/8);
      if (a<distDestAxis1) distDestAxis1=a;
    }
  }
  lastPosAxis2=a2;
  #endif

  if (distDestAxis1<1) distDestAxis1=1;
  if (distDestAxis2<1) distDestAxis2=1;

  // quickly slow the motors and stop in DegreesForRapidStop
  if (abortSlew) {
    // aborts any meridian flip
    if ((pierSideControl==PierSideFlipWE1) || (pierSideControl==PierSideFlipWE2) || (pierSideControl==PierSideFlipWE3)) pierSideControl=PierSideWest;
    if ((pierSideControl==PierSideFlipEW1) || (pierSideControl==PierSideFlipEW2) || (pierSideControl==PierSideFlipEW3)) pierSideControl=PierSideEast;
    if (pauseHome) { waitingHome=false; waitingHomeContinue=false; }

    // set the destination near where we are now
    cli();
    if (distDestAxis1>(long)(StepsPerDegreeAxis1*DegreesForRapidStop)) { if (posAxis1>(long)targetAxis1.part.m) targetAxis1.part.m=posAxis1-(long)(StepsPerDegreeAxis1*DegreesForRapidStop); else targetAxis1.part.m=posAxis1+(long)(StepsPerDegreeAxis1*DegreesForRapidStop); targetAxis1.part.f=0; }
    if (distDestAxis2>(long)(StepsPerDegreeAxis2*DegreesForRapidStop)) { if (posAxis2>(long)targetAxis2.part.m) targetAxis2.part.m=posAxis2-(long)(StepsPerDegreeAxis2*DegreesForRapidStop); else targetAxis2.part.m=posAxis2+(long)(StepsPerDegreeAxis2*DegreesForRapidStop); targetAxis2.part.f=0; }
    sei();

    if (parkStatus==Parking) {
      lastTrackingState=abortTrackingState;
      parkStatus=NotParked;
      nv.write(EE_parkStatus,parkStatus);
    } else
    if (homeMount) {
      lastTrackingState=abortTrackingState;
      homeMount=false;
    }
    
    abortSlew=false;
    goto Again;
  }

  // First, for Right Ascension
  long temp;
  if (distStartAxis1>distDestAxis1) {
    temp=(StepsForRateChangeAxis1/isqrt32(distDestAxis1));   // slow down (temp gets bigger)
  } else {
    temp=(StepsForRateChangeAxis1/isqrt32(distStartAxis1));  // speed up (temp gets smaller)
  }
  if (temp<maxRate) temp=maxRate;                            // fastest rate 
  if (temp>TakeupRate) temp=TakeupRate;                      // slowest rate
  cli(); timerRateAxis1=temp; sei();

  // Now, for Declination
  if (distStartAxis2>distDestAxis2) {
    temp=(StepsForRateChangeAxis2/isqrt32(distDestAxis2));   // slow down
  } else {
    temp=(StepsForRateChangeAxis2/isqrt32(distStartAxis2));  // speed up
  }
  if (temp<maxRate) temp=maxRate;                            // fastest rate
  if (temp>TakeupRate) temp=TakeupRate;                      // slowest rate
  cli(); timerRateAxis2=temp; sei();

#ifdef MOUNT_TYPE_ALTAZM
  // in AltAz mode if the end of slew doesn't get close enough within 3 seconds: stop tracking for a moment to allow target/actual position synchronization
  static bool forceSlewStop=false;
  static unsigned long slewStopTime=0;
  if ( (!forceSlewStop) && (distDestAxis1<=GetStepsPerSecondAxis1()) && (distDestAxis2<=GetStepsPerSecondAxis2()) ) { slewStopTime=millis()+3000L; forceSlewStop=true; }
  if ( (lastTrackingState==TrackingSidereal) && (forceSlewStop && ((long)(millis()-slewStopTime)>0)) ) lastTrackingState=TrackingSiderealDisabled;
#endif

  if ((distDestAxis1<=2) && (distDestAxis2<=2)) {
    
#ifdef MOUNT_TYPE_ALTAZM
    // if we stopped tracking turn it back on now
    if (lastTrackingState==TrackingSiderealDisabled) lastTrackingState=TrackingSidereal;
    forceSlewStop=false;
#endif
    
    if ((pierSideControl==PierSideFlipEW2) || (pierSideControl==PierSideFlipWE2)) {
      // just wait stop here until we get notification to continue
      if (pauseHome) {
        if (!waitingHomeContinue) { waitingHome=true; return; }
        soundAlert(); waitingHome=false; waitingHomeContinue=false;
      }

      // make sure we're at the home position just before flipping sides of the mount
      cli();
      startAxis1=posAxis1;
      startAxis2=posAxis2;
      sei();
      if (celestialPoleAxis1==0.0) {
        // for fork mounts
        if (pierSideControl==PierSideFlipEW2) setTargetAxis1(180.0,PierSideEast); else setTargetAxis1(-180.0,PierSideWest);
      } else {
        // for eq mounts
        if (pierSideControl==PierSideFlipEW2) setTargetAxis1(celestialPoleAxis1,PierSideEast); else setTargetAxis1(-celestialPoleAxis1,PierSideWest);
      }
//      setTargetAxis2(celestialPoleAxis2,PierSideWest);

      D("Flp2 Axis1, Current "); D(((double)(long)posAxis1)/(double)StepsPerDegreeAxis1); D(" -to-> "); DL(((double)(long)targetAxis1.part.m)/(double)StepsPerDegreeAxis1);
      D("Flp2 Axis2, Current "); D(((double)(long)posAxis2)/(double)StepsPerDegreeAxis2); D(" -to-> "); DL(((double)(long)targetAxis2.part.m)/(double)StepsPerDegreeAxis2); DL("");
      
      forceRefreshGetEqu();
      pierSideControl++;
    } else
    if ((pierSideControl==PierSideFlipEW3) || (pierSideControl==PierSideFlipWE3)) {
      
      // the blAxis2 gets "reversed" when we Meridian flip, since the NORTH/SOUTH movements are reversed
      cli(); blAxis2=backlashAxis2-blAxis2; sei();

      if (pierSideControl==PierSideFlipEW3) pierSideControl=PierSideWest; else pierSideControl=PierSideEast;
    
      // now complete the slew
      cli();
      startAxis1=posAxis1;
      targetAxis1.fixed=origTargetAxis1.fixed;
      startAxis2=posAxis2;
      targetAxis2.part.m=origTargetAxis2; targetAxis2.part.f=0;
      sei();

      forceRefreshGetEqu();
      D("Flp3 Axis1, Current "); D(((long)posAxis1)/StepsPerDegreeAxis1); D(" -to-> "); DL(((long)targetAxis1.part.m)/StepsPerDegreeAxis1);
      D("Flp3 Axis2, Current "); D(((long)posAxis2)/StepsPerDegreeAxis2); D(" -to-> "); DL(((long)targetAxis2.part.m)/StepsPerDegreeAxis2); DL("");
    } else {

      StepperModeTracking();

      // other special gotos: for parking the mount and homing the mount
      if (parkStatus==Parking) {
        // clear the backlash
        int i=parkClearBacklash(); if (i==-1) return; // working

        // stop the motor timers (except guiding)
        cli(); trackingTimerRateAxis1=0.0; trackingTimerRateAxis2=0.0; sei(); delay(11);
        
        // restore trackingState
        trackingState=lastTrackingState; lastTrackingState=TrackingNone;
        SiderealClockSetInterval(siderealInterval);

        // validate location
        byte parkPierSide=nv.read(EE_pierSide);
        if ((blAxis1!=0) || (blAxis2!=0) || (posAxis1!=(long)targetAxis1.part.m) || (posAxis2!=(long)targetAxis2.part.m) || (pierSideControl!=parkPierSide) || (i!=1)) { parkStatus=ParkFailed; nv.write(EE_parkStatus,parkStatus); }

        // sound park done
        soundAlert();

        // wrap it up
        parkFinish();
      } else
        // sound goto done
        soundAlert();
  
        // restore last tracking state
        cli();
        timerRateAxis1=SiderealRate;
        timerRateAxis2=SiderealRate;
        sei();
  
        if (homeMount) {
          // clear the backlash
          if (parkClearBacklash()==-1) return;  // working, no error flagging

          // restore trackingState
          trackingState=lastTrackingState; lastTrackingState=TrackingNone;
          SiderealClockSetInterval(siderealInterval);

          setHome();
          homeMount=false; 
          atHome=true;

          DisableStepperDrivers();
        } else {
          // restore trackingState
          trackingState=lastTrackingState; lastTrackingState=TrackingNone;
          SiderealClockSetInterval(siderealInterval);
        }
    }
  }
}

// fast integer square root routine, Integer Square Roots by Jack W. Crenshaw
uint32_t isqrt32 (uint32_t n) {
    register uint32_t root=0, remainder, place= 0x40000000;
    remainder = n;

    while (place > remainder) place = place >> 2;
    while (place) {
        if (remainder >= root + place) {
            remainder = remainder - root - place;
            root = root + (place << 1);
        }
        root = root >> 1;
        place = place >> 2;
    }
    return root;
}

