/*
        
        File:			GPSController.m
        Program:		KisMAC
		Author:			Michael Rossberg, Robin Darroch
						mick@binaervarianz.de
		Description:	KisMAC is a wireless stumbler for MacOS X.
                
        This file is part of KisMAC.
        
        Parts of this file are based on bsd airtools by h1kari.

    KisMAC is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    KisMAC is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with KisMAC; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#import "GPSController.h"
#import "WaveHelper.h"
#import "KisMACNotifications.h"
#import "Trace.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

#include <sys/termios.h>

struct termios ttyset;

#define MAX_GPSBUF_LEN 1024
#define VELOCITY_UNIT "km/h"
#define VELOCITY_CONVERSION 1.852
#define DISTANCE_UNIT "km"

@interface GPSController(PrivateExtension) 
    - (void)setStatus:(NSString*)status;
@end

@implementation GPSController

- (id)init {
    _gpsLock = [[NSLock alloc] init];
    _gpsThreadUp    = NO;
    _gpsShallRun    = NO;
    _debugEnabled   = NO;
	_gpsdReconnect	= YES;
    _lastAdd        = [[NSDate date] retain];
    _linesRead      = 0;

    [self setStatus:NSLocalizedString(@"GPS subsystem initialized but not running.", @"GPS status")];

    return self;
}

- (bool)startForDevice:(NSString*) device {
    _reliable = NO;
    _ns.dir = 'N';
    _ns.coordinates = 100;
    _ew.dir = 'E';
    _ew.coordinates = 0;
    _elev.coordinates = -10000;
    _elev.dir = 'm';
    _velkt = 0;
	_peakvel = 0;
    _veldir = -1;
    _numsat = -1;
    _hdop = 100;
	_sectordist = 0;
	_sectortime = 0;
	_totaldist = 0;

    [self stop];
    
	sleep(1);
	
	_gpsdReconnect = YES;
	
    [WaveHelper secureReplace:&_gpsDevice withObject:device];
    [WaveHelper secureRelease:&_lastUpdate];
    [WaveHelper secureRelease:&_sectorStart];
    
    if ([_gpsDevice length]==0) {
        NSLog(@"GPS integration disabled");
        [self setStatus:NSLocalizedString(@"GPS subsystem disabled.", @"GPS status")];
        return NO;
    }

    [self setStatus:NSLocalizedString(@"Starting GPS subsystem.", @"GPS status")];
    
    if ([_gpsDevice isEqualToString:@"GPSd"]) [NSThread detachNewThreadSelector:@selector(gpsThreadGPSd:) toTarget:self withObject:Nil];
    else [NSThread detachNewThreadSelector:@selector(gpsThreadSerial:) toTarget:self withObject:Nil];
    return YES;
}

#pragma mark -

- (bool)reliable {
    return _reliable;
}

- (bool)gpsRunning {
    return _gpsThreadUp;
}

- (NSString*) NSCoord {
    if (_ns.coordinates==100) return nil;
    return [NSString stringWithFormat:@"%f%c",_ns.coordinates, _ns.dir];
}

- (NSString*) EWCoord {
    if (_ns.coordinates==100) return nil;
    return [NSString stringWithFormat:@"%f%c",_ew.coordinates, _ew.dir];
}

- (NSString*) ElevCoord {
    if (_elev.coordinates==-10000) return [NSString stringWithFormat:@"No Elevation Data"];
    //NSLog([NSString stringWithFormat:@"%f",_elev.coordinates]);
    return [NSString stringWithFormat:@"%.1f %c/%.1f ft",_elev.coordinates, _elev.dir, (_elev.coordinates * 3.2808399)]; //don't know if formatting stuff is correct
}

- (NSString*) VelKt {
	float velconv,peakconv,maxconv;
	velconv = _velkt * VELOCITY_CONVERSION;
	peakconv = _peakvel * VELOCITY_CONVERSION;
	maxconv = _maxvel * VELOCITY_CONVERSION;
	if (_velkt==_maxvel) {
		if (_veldir==-1) return [NSString stringWithFormat:@"%.1f %s (%.1f kt) [MAX]",velconv,VELOCITY_UNIT,_velkt];
		return [NSString stringWithFormat:@"%.1f %s (%.1f kt) [MAX]\nTrack: %d T",velconv,VELOCITY_UNIT,_velkt,_veldir];
	} else if (_velkt==_peakvel) {
		if (_veldir==-1) return [NSString stringWithFormat:@"%.1f %s (%.1f kt) [PEAK]",velconv,VELOCITY_UNIT,_velkt];
		return [NSString stringWithFormat:@"%.1f %s (%.1f kt) [PEAK]\nTrack: %d T",velconv,VELOCITY_UNIT,_velkt,_veldir];
	} else {
		if (_veldir==-1) return [NSString stringWithFormat:@"%.1f %s (%.1f kt) [peak: %.1f, max: %.1f]",velconv,VELOCITY_UNIT,_velkt,peakconv,maxconv];
		return [NSString stringWithFormat:@"%.1f %s (%.1f kt) [peak: %.1f, max: %.1f]\nTrack: %d T",velconv,VELOCITY_UNIT,_velkt,peakconv,maxconv,_veldir];
	}
}

- (NSString*) DistStats {
	int sectortime;
	int sterror=0;
	float timeinterval;
	sectortime = (int)_sectortime;

	if (_sectorStart && (sectortime > 0)) {
		timeinterval = [[NSDate date] timeIntervalSinceDate:_sectorStart];
		sterror = sectortime - (int)timeinterval;
		// remove negative error that develops after stopping
		if ((_velkt == 0) && (sterror < 0)) sterror = 0;
	}
	
	if (sterror == 0) {
		if (sectortime > 3600) return [NSString stringWithFormat:@"Sector: %.1f %s (%.1f nm) in %d:%02d:%02d (avg: %.1f %s)\nTotal: %.1f %s (%.1f nm)",(_sectordist * VELOCITY_CONVERSION),DISTANCE_UNIT,_sectordist,(sectortime/3600),(sectortime%3600/60),(sectortime%60),(3600 * _sectordist * VELOCITY_CONVERSION)/_sectortime,VELOCITY_UNIT,(_totaldist * VELOCITY_CONVERSION),DISTANCE_UNIT,_totaldist];
		else if (sectortime > 60) return [NSString stringWithFormat:@"Sector: %.1f %s (%.1f nm) in %d:%02d (avg: %.1f %s)\nTotal: %.1f %s (%.1f nm)",(_sectordist * VELOCITY_CONVERSION),DISTANCE_UNIT,_sectordist,(sectortime/60),(sectortime%60),(3600 * _sectordist * VELOCITY_CONVERSION)/_sectortime,VELOCITY_UNIT,(_totaldist * VELOCITY_CONVERSION),DISTANCE_UNIT,_totaldist];
		else if (sectortime > 0) return [NSString stringWithFormat:@"Sector: %.1f %s (%.1f nm) in %d seconds (avg: %.1f %s)\nTotal: %.1f %s (%.1f nm)",(_sectordist * VELOCITY_CONVERSION),DISTANCE_UNIT,_sectordist,sectortime,(3600 * _sectordist * VELOCITY_CONVERSION)/_sectortime,VELOCITY_UNIT,(_totaldist * VELOCITY_CONVERSION),DISTANCE_UNIT,_totaldist];
		else return [NSString stringWithFormat:@"Total: %.1f %s (%.1f nm)",(_totaldist * VELOCITY_CONVERSION),DISTANCE_UNIT,_totaldist];
	} else {
        if (sectortime > 3600) return [NSString stringWithFormat:@"Sector: %.1f %s (%.1f nm) in %d:%02d:%02d (avg: %.1f %s) [ERROR: %ds]\nTotal: %.1f %s (%.1f nm)",(_sectordist * VELOCITY_CONVERSION),DISTANCE_UNIT,_sectordist,(sectortime/3600),(sectortime%3600/60),(sectortime%60),(3600 * _sectordist * VELOCITY_CONVERSION)/_sectortime,VELOCITY_UNIT,sterror,(_totaldist * VELOCITY_CONVERSION),DISTANCE_UNIT,_totaldist];
        else if (sectortime > 60) return [NSString stringWithFormat:@"Sector: %.1f %s (%.1f nm) in %d:%02d (avg: %.1f %s) [ERROR: %ds]\nTotal: %.1f %s (%.1f nm)",(_sectordist * VELOCITY_CONVERSION),DISTANCE_UNIT,_sectordist,(sectortime/60),(sectortime%60),(3600 * _sectordist * VELOCITY_CONVERSION)/_sectortime,VELOCITY_UNIT,sterror,(_totaldist * VELOCITY_CONVERSION),DISTANCE_UNIT,_totaldist];
        else return [NSString stringWithFormat:@"Sector: %.1f %s (%.1f nm) in %d seconds (avg: %.1f %s) [ERROR: %ds]\nTotal: %.1f %s (%.1f nm)",(_sectordist * VELOCITY_CONVERSION),DISTANCE_UNIT,_sectordist,sectortime,(3600 * _sectordist * VELOCITY_CONVERSION)/_sectortime,VELOCITY_UNIT,sterror,(_totaldist * VELOCITY_CONVERSION),DISTANCE_UNIT,_totaldist];
	}
}

- (NSString*) QualData {
	if (_numsat==-1) return [NSString stringWithFormat:@""];
	if (_hdop>=50 || _hdop==0) return [NSString stringWithFormat:@" (%d sats)",_numsat];
	return [NSString stringWithFormat:@" (%d sats, HDOP %.1f)",_numsat,_hdop];
}

- (NSString*)status {
    if (_status) return _status;
    
    if (_lastUpdate)
        if (_elev.coordinates) 
            if ((_velkt || _maxvel) && _reliable) // only report velocity if we're sure
                return [NSString stringWithFormat:@"%@: %@ %@\n%@: %@\n%@: %@\n%@\n%@: %@%@", 
                        NSLocalizedString(@"Position", "GPS status string."), 
                        [self NSCoord],[self EWCoord],
                        NSLocalizedString(@"Elevation", "GPS status string."), 
                        [self ElevCoord],
                        NSLocalizedString(@"Velocity", "GPS status string."), 
                        [self VelKt],[self DistStats],
						NSLocalizedString(@"Time", "GPS status string."), 
						[self lastUpdate],[self QualData]];
            else
                return [NSString stringWithFormat:@"%@: %@ %@\n%@: %@\n%@: %@%@%@", 
                        NSLocalizedString(@"Position", "GPS status string."), 
                        [self NSCoord],[self EWCoord],
                        NSLocalizedString(@"Elevation", "GPS status string."), 
                        [self ElevCoord],
						NSLocalizedString(@"Time", "GPS status string."), 
						[self lastUpdate],
                        _reliable ? @"" : NSLocalizedString(@" -- NO FIX", "GPS status string. Needs leading space"),
                        [self QualData]];
        else
            return [NSString stringWithFormat:@"%@: %@ %@\n%@%@", 
                NSLocalizedString(@"Position", "GPS status string."), 
                [self NSCoord],[self EWCoord],
                [self lastUpdate],
                _reliable ? @"" : NSLocalizedString(@" -- NO FIX", "GPS status string. Needs leading space"),
				[self QualData]];

    else if ([(NSString*)[[NSUserDefaults standardUserDefaults] objectForKey:@"GPSDevice"] length]) {
        if (_gpsThreadUp) return NSLocalizedString(@"GPS subsystem works, but there is no data.\nIf you are using gpsd, there may be no GPS connected.\nOtherwise, your GPS is probably connected but not yet reporting a position.", "GPS status string");
        else  return NSLocalizedString(@"GPS not working", "LONG GPS status string with informations howto debug");
            //@"GPS subsystem is not working. See log file for more details."
    } else return NSLocalizedString(@"GPS disabled", "LONG GPS status string with informations where to enable");
            //@"GPS subsystem is disabled. You have to select a device in the preferences window."
}

- (void)setStatus:(NSString*)status {
    [WaveHelper secureReplace:&_status withObject:status];
    [[NSNotificationCenter defaultCenter] postNotificationName:KisMACGPSStatusChanged object:_status];
}

- (waypoint) currentPoint {
    waypoint w;
    
    w._lat =_ns.coordinates * ((_ns.dir=='N') ? 1.0 : -1.0);
    w._long=_ew.coordinates * ((_ew.dir=='E') ? 1.0 : -1.0);
    w._elevation=_elev.coordinates;
    
    return w;
}

- (void) resetTrace {
    [[WaveHelper trace] setTrace:nil];
}

- (void)setTraceInterval:(int)interval {
    _traceInterval = interval;
}
- (void)setTripmateMode:(bool)mode {
    _tripmateMode = mode;
}

- (void) setCurrentPointNS:(double)ns EW:(double)ew ELV:(double)elv{  //need to add elevation support here
    waypoint w;
    _ns.dir = (ns<0 ? 'S' : 'N');
    _ew.dir = (ew<0 ? 'W' : 'E');
    
    _ns.coordinates = fabs(ns);
    _ew.coordinates = fabs(ew); 
    
    [WaveHelper secureReplace:&_lastUpdate withObject:[NSDate date]];
    [WaveHelper secureReplace:&_lastAdd withObject:[NSDate date]];
    
    if (abs(ns)>=0 && abs(ns)<=90 && abs(ew)>=0 && abs(ew)<=180) {
        w._long = ew;
        w._lat  = ns;
        [[WaveHelper trace] addPoint:w];
    }
}

- (void)setOnNoFix:(int)onNoFix {
    _onNoFix=onNoFix;
}

- (NSDate*) lastUpdate {
    return _lastUpdate;
}

#pragma mark -

bool check_sum(char *s, char h, char l) {
  char checksum;
  unsigned char ref;      /* must be unsigned */

#ifdef PARANOIA
  if(!s)
    return NO;
  if(!*s)
    return NO;
#endif

  checksum = *s++;
  while(*s && *s !='*')
    checksum ^= *s++;

#ifdef PARANOIA
  if(!isxdigit(h))
    return NO;
  if(!isxdigit(l))
    return NO;
  h = (char)toupper(h);
  l = (char)toupper(l);
#endif

  ref =  ((h >= 'A') ? (h -'A' + 10):(h - '0'));
  ref <<= 4;
  ref &= ((l >= 'A') ? (l -'A' + 10):(l - '0'));

  if((char)ref == checksum)
    return YES;             /* ckecksum OK */
  
  return NO;              /* checksum error */
}

int ss(char* inp, char* outp) {
    int x=0;
    
    while(true) {
        if (inp[x]==0) return -1;
        if (inp[x]=='\n') {
            outp[x]=0;
            return x;
        }
        outp[x]=inp[x];
        x++;
    }
    
    return x;
}

- (bool)gps_parse:(int) fd {
    int len, valid, x=0;
    static int q = 0;
    char cvalid;
    static char gpsin[MAX_GPSBUF_LEN];
    char gpsbuf[MAX_GPSBUF_LEN];
    int ewh, nsh;
	int veldir,numsat;
	float velkt,hdop;
	float timeinterval=-1;
	float displacement;
    struct _position ns, ew, elev;
    bool updated;
    NSDate *date;
    NSAutoreleasePool* subpool = [[NSAutoreleasePool alloc] init];

    if (_debugEnabled) NSLog(@"GPS read data");
    if (q>=1024) q = 0; //just in case something went wrong
    
    if((len = read(fd, &gpsin[q], MAX_GPSBUF_LEN-q-1)) < 0) return NO;
    if (len == 0) return YES;
    
    if (_debugEnabled) NSLog(@"GPS read data returned.");
    [self setStatus:nil];
    _linesRead++;
    
    gpsin[q+len]=0;
    updated = NO;
    elev.coordinates = -10000.0;
	velkt = -1.0;
	numsat = -1;
	hdop = 100;
    
    while (ss(&gpsin[x],gpsbuf)>0) {
        if (_debugEnabled) NSLog(@"GPS record: %s", gpsbuf);//uncommented
        if(_tripmateMode && (!strncmp(gpsbuf, "ASTRAL", 6))) {
            write(fd, "ASTRAL\r", 7);
        } else if(strncmp(gpsbuf, "$GPGGA", 6) == 0) {  //gpsbuf contains GPS fixed data (almost everything poss)
            if (sscanf(gpsbuf, "%*[^,],%*f,%2d%f,%c,%3d%f,%c,%d,%d,%f,%f",
		&nsh, &ns.coordinates, &ns.dir,
                &ewh, &ew.coordinates, &ew.dir,
	        &valid, &numsat, &hdop, &elev.coordinates)>=7) { // this probably should be == 10 not >= 7  more testing
                		
                if (valid) _reliable = YES;
                else _reliable = NO;
                
                if (_debugEnabled) NSLog(@"GPS data updated.");
                updated = YES;
            }
        } else if(strncmp(gpsbuf, "$GPRMC", 6) == 0) {  //gpsbuf contains Recommended minimum specific GPS/TRANSIT data !!does not include elevation
            if (sscanf(gpsbuf, "%*[^,],%*f,%c,%2d%f,%c,%3d%f,%c,%f,%d,",
                &cvalid, &nsh, &ns.coordinates, &ns.dir,
                &ewh, &ew.coordinates, &ew.dir, &velkt, &veldir)==9) {
            
                if (cvalid == 'A') _reliable = YES;
                else _reliable = NO;
                
                if (_debugEnabled) NSLog(@"GPS data updated.");  
                updated = YES;
            }
        } else if(strncmp(gpsbuf, "$GPGLL", 6) == 0) {  //gbsbuf contains Geographical postiion, latitude and longitude only  !!does not include elevation
            if (sscanf(gpsbuf, "%*[^,],%2d%f,%c,%3d%f,%c,%*f,%c",
                &nsh, &ns.coordinates, &ns.dir,
                &ewh, &ew.coordinates, &ew.dir, &cvalid)==7) {
            
                if (cvalid == 'A') _reliable = YES;
                else _reliable = NO;
                
                if (_debugEnabled) NSLog(@"GPS data updated.");  
                updated = YES;
            }
        }
        
        x+=strlen(gpsbuf)+1;
    }
    
    q+=len-x;
    memcpy(gpsbuf,&gpsin[x],q);
    memcpy(gpsin,gpsbuf,q);
    if (q>80) q=0;
    
	date = [[NSDate alloc] init];
    
	if (updated) {
		timeinterval = [date timeIntervalSinceDate:_lastUpdate];

        if ((_reliable)||(_onNoFix==0)) {
            if (ns.dir != 'S') _ns.dir = 'N';
            else _ns.dir = 'S';
            
            if (ew.dir != 'W') _ew.dir = 'E';
            else _ew.dir = 'W';
            
            _ns.coordinates   = nsh + ns.coordinates / 60.0;
            _ew.coordinates   = ewh + ew.coordinates / 60.0;
            if (elev.coordinates > -10000.00) _elev.coordinates = elev.coordinates;
			
            if (velkt > -1.0) {
				if ((velkt > 0) && (_velkt==0)) {
					_peakvel = 0;
					_sectordist = 0;
					_sectortime = 0;
					[WaveHelper secureReplace:&_sectorStart withObject:date];
				} else if ((velkt > 0) || (_velkt > 0)) {
					// update distances only if we're moving (or just stopped)
					displacement = (velkt + _velkt)*timeinterval/7200;
					_sectordist += displacement;
					_sectortime += timeinterval;
					_totaldist += displacement;
				}
                _velkt = velkt;
                _veldir = veldir;
				if (velkt > _peakvel) _peakvel = velkt;
				if (velkt > _maxvel) _maxvel = velkt;
			}
            
            if (numsat > -1) {
                _numsat = numsat;
                _hdop = hdop;
            }
        } else if(_onNoFix==2) {
            _ns.dir = 'N';
            _ew.dir = 'E';
            
            _elev.coordinates = -10000;
            _ns.coordinates = 100;
            _ew.coordinates = 0;
            _velkt = 0;
        }

        [WaveHelper secureReplace:&_lastUpdate withObject:date];

        if (_reliable) {
            if (([_lastUpdate timeIntervalSinceDate:_lastAdd]>_traceInterval) && (_traceInterval != 100)) {
                waypoint w;
                w._lat  = _ns.coordinates * ((_ns.dir=='N') ? 1.0 : -1.0);
                w._long = _ew.coordinates * ((_ew.dir=='E') ? 1.0 : -1.0);
                if ([[WaveHelper trace] addPoint:w]) [WaveHelper secureReplace:&_lastAdd withObject:date];
            }
        } else {
            [[WaveHelper trace] cut];
        }
    }
    
    [date release];
    [subpool release];

    return YES;
}

- (bool)gpsd_parse:(int) fd {
    int len, valid, numsat, veldir;
    char gpsbuf[MAX_GPSBUF_LEN];
    double ns, ew, elev;
	float velkt,hdop,fveldir;
	float timeinterval=-1;
	float displacement;
    NSDate *date;
    NSAutoreleasePool* subpool = [[NSAutoreleasePool alloc] init];

    if (_debugEnabled) NSLog(@"GPSd write command");
    
    if (write(fd, "PMVTAQ\r\n", 8) < 8) {
        NSLog(@"GPSd write failed");
        return NO;
    }
    
    if((len = read(fd, &gpsbuf[0], MAX_GPSBUF_LEN)) < 0) {
        NSLog(@"GPSd read failed");
        return NO;
    }
    if (len == 0) return YES;
    
    if (_debugEnabled) NSLog(@"GPSd read data returned.");
    [self setStatus:nil];
    _linesRead++;
    
    gpsbuf[0+len]=0;
 	numsat = -1;
	hdop = 100;
	elev = 0;
	
	date = [[NSDate alloc] init];

	if (sscanf(gpsbuf, "GPSD,P=%lg %lg,M=%d,V=%f,T=%f,A=%lg,Q=%d %*f %f",
        &ns, &ew, &valid, &velkt, &fveldir, &elev, &numsat, &hdop) >=4) {
                    
        if (valid >= 2) _reliable = YES;
        else _reliable = NO;
        
        if (_debugEnabled) NSLog(@"GPSd data updated.");
		
		timeinterval = [date timeIntervalSinceDate:_lastUpdate];
        [WaveHelper secureReplace:&_lastUpdate withObject:date];


        if ((_reliable)||(_onNoFix==0)) {
            if (ns >= 0) _ns.dir = 'N';
            else _ns.dir = 'S';
            
            if (ew >= 0) _ew.dir = 'E';
            else _ew.dir = 'W';
            
            _ns.coordinates   = fabs(ns);
            _ew.coordinates   = fabs(ew);
            _elev.coordinates = elev;
			if ((velkt > 0) && (_velkt==0)) {
				_peakvel = 0;
				_sectordist = 0;
				_sectortime = 0;
				[WaveHelper secureReplace:&_sectorStart withObject:date];
			} else if ((velkt > 0) || (_velkt > 0)) {
				// update distances only if we're moving (or just stopped)
				displacement = (velkt + _velkt)*timeinterval/7200;
				_sectordist += displacement;
				_sectortime += timeinterval;
				_totaldist += displacement;
			}
			_velkt = velkt;
			veldir = (int)fveldir;
			_veldir = veldir;
			if (velkt > _peakvel) _peakvel = velkt;
			if (velkt > _maxvel) _maxvel = velkt;

			if (numsat > -1) {
				_numsat = numsat;
				_hdop = hdop;
			}
		} else if(_onNoFix==2) {
            _ns.dir = 'N';
            _ew.dir = 'E';
            
            _elev.coordinates = -10000;
            _ns.coordinates = 100;
            _ew.coordinates = 0;
            _velkt = 0;
        }

		if (_reliable) {
            if (([_lastUpdate timeIntervalSinceDate:_lastAdd]>_traceInterval) && (_traceInterval != 100)) {
                waypoint w;
                w._lat  = _ns.coordinates * ((_ns.dir=='N') ? 1.0 : -1.0);
                w._long = _ew.coordinates * ((_ew.dir=='E') ? 1.0 : -1.0);
                if ([[WaveHelper trace] addPoint:w]) [WaveHelper secureReplace:&_lastAdd withObject:date];
            }
        } else {
            [[WaveHelper trace] cut];
        }

    } else {
// be quiet for now
//        NSLog(@"GPSd parsing failure - received: %s",gpsbuf);
    }
    
    [date release];
    [subpool release];

    return YES;
}

- (void) continousParse:(int) fd {
    NSDate *date;
    unsigned int i = 0;
    NSAutoreleasePool* subpool;
	
    while (_gpsShallRun && [self gps_parse:fd]) {
		subpool = [[NSAutoreleasePool alloc] init];
        //actually once a sec should be enough, but sometimes we dont get any information. so do it more often.
        if ((i++ % 10 == 0) && (_status == Nil))
            [[NSNotificationCenter defaultCenter] postNotificationName:KisMACGPSStatusChanged object:[self status]];
        date = [[NSDate alloc] initWithTimeIntervalSinceNow:0.1];
        [NSThread sleepUntilDate:date];
        [date release];
		[subpool release];
    }
}

- (void) continousParseGPSd:(int) fd {
    NSDate *date;
    unsigned int i = 0;
	NSAutoreleasePool* subpool;

    while (_gpsShallRun && [self gpsd_parse:fd]) {
		subpool = [[NSAutoreleasePool alloc] init];
        if ((i++ % 2 == 0) && (_status == Nil))
            [[NSNotificationCenter defaultCenter] postNotificationName:KisMACGPSStatusChanged object:[self status]];
        date = [[NSDate alloc] initWithTimeIntervalSinceNow:0.5];
        [NSThread sleepUntilDate:date];
        [date release];
		[subpool release];
    }
}

- (void)gpsThreadSerial:(id)object {
    NSAutoreleasePool* subpool = [[NSAutoreleasePool alloc] init];
    int     handshake;
    struct  termios backup;

    _gpsShallRun = NO;
    
    if ([_gpsLock lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:10]]) {
        _gpsThreadUp = YES;
        _gpsShallRun = YES;
        [_lastUpdate release];
        _lastUpdate = nil;
        [_sectorStart release];
        _sectorStart = nil;
        
        [self setStatus:NSLocalizedString(@"GPS subsystem starting up.", @"GPS status")];

        //NSLog(@"Starting GPS device");
        if((_serialFD = open([_gpsDevice cString], O_RDWR | O_NOCTTY | O_NONBLOCK )) < 0) {
            NSLog(@"error: unable to open gps device: %s", strerror(errno));
            [self setStatus:NSLocalizedString(@"Could not open GPS.", @"GPS status")];
        } else if(!isatty(_serialFD)) {
            NSLog(@"error: specified gps device is not a tty: %s", strerror(errno));
        } else if (ioctl(_serialFD, TIOCEXCL) == -1) {
            NSLog(@"error: could not set exclusive flag: %s", strerror(errno));
        } else if (fcntl(_serialFD, F_SETFL, 0) == -1) {
            NSLog(@"error: clearing O_NONBLOCK: %s(%d).\n", strerror(errno), errno);
        } else if(tcgetattr(_serialFD, &backup) != 0) {
            NSLog(@"error: unable to set attributes for gps device: %s", strerror(errno));
        } else if(ioctl(_serialFD, TIOCGETA, &ttyset) < 0) {
            NSLog(@"error: unable to ioctl gps device: %s", strerror(errno));
        } else {
            //NSLog(@"GPS device is open");
            ttyset.c_ispeed = B4800;
            ttyset.c_ospeed = B4800;
            
            ttyset.c_cflag |=       CRTSCTS;    // hadware flow on
            ttyset.c_cflag &=       ~PARENB;    // no parity
            ttyset.c_cflag &=       ~CSTOPB;    // one stopbit
            ttyset.c_cflag &=       CSIZE;
            ttyset.c_cflag |=       CS8;        // 8N1
            ttyset.c_cflag |=       (CLOCAL | CREAD); //enable Localmode, receiver
            ttyset.c_cc[VMIN] =     20;         // set min read chars if 0  VTIME takes over
            ttyset.c_cc[VTIME] =    10;         // wait x ms for charakter

            //options.c_cflag &= ~ ICANON; // canonical input 
            ttyset.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

            
            if(ioctl(_serialFD, TIOCSETAF, &ttyset) < 0) {
                NSLog(@"error: unable to ioctl gps device: %s", strerror(errno));
            } else {
                if (ioctl(_serialFD, TIOCSDTR) == -1) { // Assert Data Terminal Ready (DTR)
                    NSLog(@"Error asserting DTR - %s(%d).\n", strerror(errno), errno);
                }
                
                if (ioctl(_serialFD, TIOCCDTR) == -1) { // Clear Data Terminal Ready (DTR) 
                    NSLog(@"Error clearing DTR - %s(%d).\n", strerror(errno), errno);
                }
                
                handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
                if (ioctl(_serialFD, TIOCMSET, &handshake) == -1) { // Set the modem lines depending on the bits set in handshake
                    NSLog(@"Error setting handshake lines - %s(%d).\n", strerror(errno), errno);
                }
                
                if (ioctl(_serialFD, TIOCMGET, &handshake) == -1) { // Store the state of the modem lines in handshake
                    NSLog(@"Error getting handshake lines - %s(%d).\n", strerror(errno), errno);
                }

                NSLog(@"GPS started successfully in serial mode\n");
                [self setStatus:NSLocalizedString(@"GPS started in serial mode.", @"GPS status")];

                [self continousParse:_serialFD];
            }
            
            if (_serialFD) close(_serialFD);
            [self setStatus:NSLocalizedString(@"GPS device closed.", @"GPS status")];
        }    

        [_gpsLock unlock];
        _gpsThreadUp = NO;
    } else {
        NSLog(@"GPS LOCKING FAILURE!");
    }
    
    [subpool release];
    return;
}

- (void)gpsThreadGPSd:(id)object {
    int sockd;
    struct sockaddr_in serv_name;
    int status;
    struct hostent *hp;
    UInt32 ip;
    NSUserDefaults *sets;
    const char *hostname;
    NSAutoreleasePool* subpool = [[NSAutoreleasePool alloc] init];
    
    _gpsShallRun = NO;
    
    if ([_gpsLock lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:10]]) {
        _gpsThreadUp = YES;
        _gpsShallRun = YES;
        [_lastUpdate release];
        _lastUpdate = nil;
        [_sectorStart release];
        _sectorStart = nil;
        
        [self setStatus:NSLocalizedString(@"Starting GPS in GPSd mode.", @"GPS status")];

        sets = [NSUserDefaults standardUserDefaults];
        
		while(_gpsdReconnect) {
			sockd  = socket(AF_INET, SOCK_STREAM, 0);
			if (sockd == -1) {
				NSLog(@"Socket creation failed!");
				[self setStatus:NSLocalizedString(@"Could not create GPSd socket.", @"GPS status")];
				goto err;
			}
			
			hostname = [[sets objectForKey:@"GPSDaemonHost"] cString];
			
			if (inet_addr(hostname) != INADDR_NONE) {
				ip = inet_addr(hostname);
			} else {
				hp = gethostbyname(hostname);
				if (hp == NULL) {
					NSLog(@"Could not resolve %s", hostname);
					[self setStatus:NSLocalizedString(@"Could not resolve GPSd server.", @"GPS status")];
					goto err;
				}
				ip = *(int *)hp->h_addr_list[0];
			}
			
			/* server address */
			serv_name.sin_addr.s_addr = ip;
			serv_name.sin_family = AF_INET;
			serv_name.sin_port = htons([sets integerForKey:@"GPSDaemonPort"]);

			NSLog(@"Connecting to gpsd (%s)",inet_ntoa(serv_name.sin_addr));

			/* connect to the server */
			status = connect(sockd, (struct sockaddr*)&serv_name, sizeof(serv_name));
			
			if (status == -1) {
				NSLog(@"Could not connect to %s port %d", hostname, [sets integerForKey:@"GPSDaemonPort"]);
				[self setStatus:NSLocalizedString(@"Could not connect to GPSd.", @"GPS status")];
				goto err;
			}

			NSLog(@"GPS started successfully in GPSd mode.\n");
			[self setStatus:NSLocalizedString(@"GPS started in GPSd mode.", @"GPS status")];

			[self continousParseGPSd: sockd];
			close(sockd);

			[self setStatus:NSLocalizedString(@"GPSd connection terminated - reconnecting...", @"GPS status")];
		}
    err:
        [_gpsLock unlock];
        _gpsThreadUp = NO;
    } else {
        NSLog(@"GPS LOCKING FAILURE!");
    }

    [subpool release];
    return;
}

#pragma mark -

- (void)writeDebugOutput:(BOOL)enable {
    _debugEnabled = enable;
}

#pragma mark -

- (void)stop {
    int fd;
    _gpsShallRun=NO;
	_gpsdReconnect=NO;

    [self setStatus:NSLocalizedString(@"Trying to terminate GPS subsystem.", @"GPS status")];
    
    if ([_gpsLock lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:0.5]]) {
        [_gpsLock unlock];
    } else {
        //kill the file descriptor if cannot obtain a lock
        if (_serialFD) {
            fd = _serialFD;
            _serialFD = 0;
            close(fd);
        }
    }
}

- (void) dealloc {
    [WaveHelper secureRelease:&_status];
    _gpsShallRun=NO;
    [_gpsLock release];
    [_gpsDevice release];
    [_lastAdd release];
    [super dealloc];
}

@end
