module BACnetAPDU;

@load bacnet_apdu_types.bro

#################################################################
#								#
#			   BACNET LOGs				#
#								#
#################################################################

export {

  redef enum Log::ID += { INFORMATION_UNITS };

  type Info_INFORMATION_UNITS: record {

    timestamp:		string 	&log;

    message:		string 	&log;
    
    objectid:		string 	&log;
    objectinstance:	count 	&log;
    
    property:		string 	&log; 
    value:		string 	&log;
  
  };

  redef enum Log::ID += { ERRORS };

  type Info_ERRORS: record {

    timestamp:	string 	&log;

    reason:	string 	&log;
    
    invokeid:	count 	&log;
    service:	string 	&log;
    
    info:	string 	&log	&optional;
  
  };

}

event bro_init()
{
  Log::create_stream(INFORMATION_UNITS, [$columns=Info_INFORMATION_UNITS]);
  Log::create_stream(ERRORS, [$columns=Info_ERRORS]);
}

################################################################################################

#
# INFO: This set of instructions allows to parse all the resources within a BACnet message
#

type Resource : record {

  tagPath: string;		# Tag parsed to reach the resource
  tagLabel: string;		# Resource type
  value: any;			# Resource value

};

global resources: vector of Resource;							# This variables stores all parsed Resources

event bacnet_message(c: connection, is_orig: bool, func: count, len: count)		# This event empties the "resources" buffer
{
  resources = vector();
}

################################################################################################

#
# INFO: This set of instructions allows to link BACnet requests and responses
#

export {

  type InformationUnit : record {

    message:	string;
    object:	BACnetAPDUTypes::BACnetObjectIdentifier		&optional;
    property:	string						&optional;
    value:	any						&optional;

  };

  type Request_Info : record {

    service:		string;
    content:		any			&optional;				# INFO: This variable stores data exchanged in the BACnet request (e.g., ReadProperty_Request_Info, ReadPropertyMultiple_Request_Info, etc.)
	
  };

}

global requests : table[string] of Request_Info;				# INFO: This variable stores all requests that have not been matched yet # FIXME: Indexing information may use IP addresses, but right now just uses invokeIDs and BACnet services

function pushRequestInfo(invokeid: count, service: string, content: any)	# INFO: Given a request, this function stores the related information
{
  
  local key : string = fmt("%s_%s", service, invokeid);

  if(key in requests){
    
    Log::write(BACnetAPDU::ERRORS, [$timestamp = fmt("%s", network_time()), $reason = "\"Missing ACK/Retransmission\"", $invokeid = invokeid, $service = service, $info = fmt("\"%s\"", requests[key])]);
  
  }
  
  requests[key] = [$service = service, $content = content];

}

function popRequestInfo(invokeid: count, service: string): Request_Info 	# INFO: Given a response, this function returns the related information in the request and deletes it
{

  local key : string = fmt("%s_%s", service, invokeid);

  if(key in requests){
    
    local request_info: Request_Info = requests[key];
    delete requests[key];
    
    return request_info;
    
  } else {

    Log::write(BACnetAPDU::ERRORS, [$timestamp = fmt("%s", network_time()), $reason = "\"Missing Request\"", $invokeid = invokeid, $service = service]);
    
    return [$service = ""];

  }

}

################################################################################################

#
# INFO: This function allows to extract an information unit from any BACnet request
#

export {

  global extractInformationUnit: function(service: string, content: any, isrequest: bool &default = T): InformationUnit;

}

#################################################################
#								#
#		    BACNET PRIMITIVE TYPES			#
#								#
#################################################################

type Any : record {

  label: string;
  value: any;

};

####################################
#           Event: Null            #
####################################
type Null : record {

  label: string;

};

event bacnet_null(path: string, label: string) &priority = 5
{}

####################################
#         Event: Boolean           #
####################################
type Boolean : record {

  label: string;
  value: bool;

};

event bacnet_boolean(path: string, label: string, value: any) &priority = 5
{

  local b: Boolean = [$label = "boolean", $value = value];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = b];

}

####################################
#         Event: Unsigned          #
####################################
type Unsigned : record {

  label: string;
  value: count;

};

event bacnet_unsigned(path: string, label: string, value: count) &priority = 5
{

  local u: Unsigned = [$label = "unsigned", $value = value];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = u];

}

####################################
#          Event: Integer          #
####################################
type Integer : record {

  label: string;
  value: int;

};

event bacnet_integer(path: string, label: string, value: any) &priority = 5
{

  local i: Integer = [$label = "integer", $value = value];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = i];

}

####################################
#           Event: Real            #
####################################
type Real : record {

  label: string;
  value: double;

};

event bacnet_real(path: string, label: string, value: any) &priority = 5
{

  local r: Real = [$label = "real", $value = value];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = r];

}

####################################
#          Event: Double           #
####################################
type Double : record {

  label: string;
  value: double;

};

event bacnet_double(path: string, label: string, value: any) &priority = 5
{

  local d: Double = [$label = "double", $value = value];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = d];

}

####################################
#          Event: Octets           #
####################################
type Octets : record {

  label: string;
  value: string;									# INFO: This could be something different

};

event bacnet_octets(path: string, label: string, value: any) &priority = 5
{

  local o: Octets = [$label = "octets", $value = value];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = o];

}  

####################################
#     Event: Character String      #
####################################
type CharacterString : record {

  label: string;
  value: string;

};

event bacnet_character_string(path: string, label: string, value: string) &priority = 5
{

  local cs: CharacterString = [$label = "characterstring", $value = value];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = cs];

}

####################################
#       Event: Bit String          #
####################################
type BitString : record {

  label: string;
  value: vector of bool;

};

event bacnet_bit_string(path: string, label: string, unused: count, values: vector of bool) &priority = 5
{

  local bs: BitString = [$label = "bitstring", $value = vector()];

  for (i in values) {
    if( i < |values| - unused ){
      bs$value[|bs$value|] = values[i];
    }
  }

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = bs];

}

####################################
#       Event: Enumerated          #
####################################
type Enumerated : record {

  label: string;
  value: count;

};

event bacnet_enumerated(path: string, label: string, value: count) &priority = 5
{

  local e: Enumerated = [$label = "enumerated", $value = value];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = e];

}

####################################
#           Event: Date            #
####################################
type Date : record {
  
  label: string;
  value: BACnetAPDUTypes::Date;

};

event bacnet_date(path: string, label: string, year: count, month: count, day: count, weekday: count) &priority = 5
{

  local d: BACnetAPDUTypes::Date = [$year = year, $month = month, $day = day, $weekday = weekday];
  local date: Date = [$label = "date", $value = d];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = date];

}

####################################
#          Event: Time             #
####################################
type Time : record {
    
  label: string;
  value: BACnetAPDUTypes::Time;

};

event bacnet_time(path: string, label: string, hour: count, minute: count, second: count, centisecond: count) &priority = 5
{

  local t: BACnetAPDUTypes::Time = [$hour = hour, $minute = minute, $second = second, $centisecond = centisecond];
  local time_: Time = [$label = "time", $value = t];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = time_];

}

####################################
#       Event: BACnetObject        #
####################################
type Object : record {

  label: string;
  value: BACnetAPDUTypes::BACnetObjectIdentifier;

};

event bacnet_object(path: string, label: string, objectid: string, objectinstancenumber: count) &priority = 5
  {
  
  local bacnetObject: BACnetAPDUTypes::BACnetObjectIdentifier = [$id = objectid, $instance = objectinstancenumber];
  local object: Object = [$label = "object", $value = bacnetObject];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = object];

  }

####################################
#      Event: BACnetProperty       #
####################################
type Property : record {

  label: string;
  value: string;

};

event bacnet_property(path: string, label: string, propertyid: string) &priority = 5
  {
  
  local property : Property = [$label = "property", $value = propertyid];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = property];

  }

####################################
#          Event: Unknown          #
####################################
type Unknown : record {

  label: string;
  value: any;

};

event bacnet_unknown(path: string, label: string, value: any) &priority = 5
{

  local u: Any = [$label = "unknown", $value = value];

  resources[|resources|] = [$tagPath = path, $tagLabel = label, $value = u];

}

#################################################################
#								#
#		    BACNET COMPOSITE TYPES			#
#								#
#################################################################

function parseBACnetDateTime(index: int, tagPath: string): BACnetAPDUTypes::BACnetDateTime
{
  
  local dateTime : BACnetAPDUTypes::BACnetDateTime;

  if(resources[index]$tagPath[|tagPath|:|resources[index]$tagPath|] == "" && resources[index]$tagLabel == "Date"){
    
    local d : Date = resources[index]$value;
    dateTime$date = d$value;

  } 
 
  if(resources[index + 1]$tagPath[|tagPath|:|resources[index + 1]$tagPath|] == "" && resources[index + 1]$tagLabel == "Time"){
    
    local t : Time = resources[index + 1]$value;
    dateTime$time_ = t$value;

  }

  return dateTime;

}

function parseBACnetTimeStamps(index: int, tagPath: string, items: count &default = 1): any
{

  local timestamps : vector of any;
 
  while(items > 0){ 
  
    if(resources[index]$tagPath[|tagPath|:|resources[index]$tagPath|] == "[0]"){
        
      local t : Time = resources[index]$value;
      timestamps[|timestamps|] = t$value;

    } else if(resources[index]$tagPath[|tagPath|:|resources[index]$tagPath|] == "[1]"){
        
      local t2 : Unsigned = resources[index]$value;
      timestamps[|timestamps|] = t2$value;

    } else if(resources[index]$tagPath[|tagPath|:|resources[index]$tagPath|] == "[2]" && resources[index]$tagLabel == "Date"){

      items = items + 1;

      local t3 : BACnetAPDUTypes::BACnetDateTime = parseBACnetDateTime(index, fmt("%s[2]", tagPath));
      timestamps[|timestamps|] = t3;

    }

    items = items - 1;
    index = index + 1;

  }

  return timestamps;

}

function parseBACnetNotificationParameters(index: int, tagPath: string): any			# FIXME: This should deal with type "BACnetNotificationParameters"
{

  local notificationParameters: vector of any;

  while(|resources| > index && resources[index]$tagPath[0:|tagPath|] == tagPath){

    notificationParameters[|notificationParameters|] = resources[index]$value;

    index = index + 1;

  }

  return notificationParameters;

}

function parseError(index: int, tagPath: string): BACnetAPDUTypes::Error
{
  
  local err : BACnetAPDUTypes::Error;

  if(resources[index]$tagPath[|tagPath|:|resources[index]$tagPath|] == "" && resources[index]$tagLabel == "ENUMERATED"){
    
    local eclass : Enumerated = resources[index]$value;
    err$error_class = BACnetAPDUTypes::Error_Class[eclass$value];

  } 

  if(resources[index + 1]$tagPath[|tagPath|:|resources[index + 1]$tagPath|] == "" && resources[index + 1]$tagLabel == "ENUMERATED"){
    
    local ecode : Enumerated = resources[index + 1]$value;
    err$error_code =  BACnetAPDUTypes::Error_Code[ecode$value];

  }

  return err;

}

function parseAbstractSyntaxAndType(index: int, tagPath: string, objectId: string &default = "", propertyId: string &default = ""): any
{

  local entry : string = fmt("%s/%s", objectId, propertyId);
  local values : vector of any;

  while(|resources| > index && resources[index]$tagPath[0:|tagPath|] == tagPath){	# TODO: This loop is not necessary unless specific properties need it
    
    switch (entry) {				# INFO:	This should have an entry for every "object/property" pair in BACnet that does not rely on an application tag

      case "analog-input/status-flags":
        local bitstring_sf : BitString = resources[index]$value;
        local analog_value__status_flags : BACnetAPDUTypes::BACnetStatusFlags;
        if(|bitstring_sf$value| > 0){analog_value__status_flags$in_alarm =		bitstring_sf$value[0];}
        if(|bitstring_sf$value| > 1){analog_value__status_flags$fault =			bitstring_sf$value[1];}
        if(|bitstring_sf$value| > 2){analog_value__status_flags$overridden =		bitstring_sf$value[2];}
        if(|bitstring_sf$value| > 3){analog_value__status_flags$out_of_service =	bitstring_sf$value[3];}
        return analog_value__status_flags;

      case "analog-input/event-state":
        local analog_input__event_state : Enumerated = resources[index]$value;
        return BACnetAPDUTypes::BACnetEventState[analog_input__event_state$value];

      case "analog-value/status-flags":
        local bitstring_sf2 : BitString = resources[index]$value;
        local analog_input__status_flags : BACnetAPDUTypes::BACnetStatusFlags;
        if(|bitstring_sf2$value| > 0){analog_input__status_flags$in_alarm =		bitstring_sf2$value[0];}
        if(|bitstring_sf2$value| > 1){analog_input__status_flags$fault =		bitstring_sf2$value[1];}
        if(|bitstring_sf2$value| > 2){analog_input__status_flags$overridden =		bitstring_sf2$value[2];}
        if(|bitstring_sf2$value| > 3){analog_input__status_flags$out_of_service =	bitstring_sf2$value[3];}
        return analog_input__status_flags;

      case "analog-value/event-state":
        local analog_value__event_state : Enumerated = resources[index]$value;
        return BACnetAPDUTypes::BACnetEventState[analog_value__event_state$value];

      case "binary-input/status-flags":
        local bitstring_sf3 : BitString = resources[index]$value;
        local binary_input__status_flags : BACnetAPDUTypes::BACnetStatusFlags;
        if(|bitstring_sf3$value| > 0){binary_input__status_flags$in_alarm =		bitstring_sf3$value[0];}
        if(|bitstring_sf3$value| > 1){binary_input__status_flags$fault =		bitstring_sf3$value[1];}
        if(|bitstring_sf3$value| > 2){binary_input__status_flags$overridden =		bitstring_sf3$value[2];}
        if(|bitstring_sf3$value| > 3){binary_input__status_flags$out_of_service =	bitstring_sf3$value[3];}
        return binary_input__status_flags;

      case "binary-input/event-state":
        local binary_input__event_state : Enumerated = resources[index]$value;
        return BACnetAPDUTypes::BACnetEventState[binary_input__event_state$value];

      case "binary-value/status-flags":
        local bitstring_sf4 : BitString = resources[index]$value;
        local binary_value__status_flags : BACnetAPDUTypes::BACnetStatusFlags;
        if(|bitstring_sf4$value| > 0){binary_value__status_flags$in_alarm =		bitstring_sf4$value[0];}
        if(|bitstring_sf4$value| > 1){binary_value__status_flags$fault =		bitstring_sf4$value[1];}
        if(|bitstring_sf4$value| > 2){binary_value__status_flags$overridden =		bitstring_sf4$value[2];}
        if(|bitstring_sf4$value| > 3){binary_value__status_flags$out_of_service =	bitstring_sf4$value[3];}
        return binary_value__status_flags;

      case "binary-value/event-state":
        local binary_value__event_state : Enumerated = resources[index]$value;
        return BACnetAPDUTypes::BACnetEventState[binary_value__event_state$value];

      case "device/system-status":
        local device__system_status : Enumerated = resources[index]$value;
        return BACnetAPDUTypes::BACnetDeviceStatus[device__system_status$value];

      case "device/protocol-services-supported":
        local bitstring_pss : BitString = resources[index]$value;
        local device__protocol_services_supported : BACnetAPDUTypes::BACnetServicesSupported;
        if(|bitstring_pss$value| > 0){device__protocol_services_supported$acknowledgeAlarm =			bitstring_pss$value[0];}
        if(|bitstring_pss$value| > 1){device__protocol_services_supported$confirmedCOVNotification =		bitstring_pss$value[1];}
        if(|bitstring_pss$value| > 2){device__protocol_services_supported$confirmedEventNotification =		bitstring_pss$value[2];}
        if(|bitstring_pss$value| > 3){device__protocol_services_supported$getAlarmSummary =			bitstring_pss$value[3];}
        if(|bitstring_pss$value| > 4){device__protocol_services_supported$getEnrollmentSummary =		bitstring_pss$value[4];}
        if(|bitstring_pss$value| > 5){device__protocol_services_supported$subscribeCOV =			bitstring_pss$value[5];}
        if(|bitstring_pss$value| > 6){device__protocol_services_supported$atomicReadFile	=		bitstring_pss$value[6];}
        if(|bitstring_pss$value| > 7){device__protocol_services_supported$atomicWriteFile =			bitstring_pss$value[7];}
        if(|bitstring_pss$value| > 8){device__protocol_services_supported$addListElement =			bitstring_pss$value[8];}
        if(|bitstring_pss$value| > 9){device__protocol_services_supported$removeListElement =			bitstring_pss$value[9];}
        if(|bitstring_pss$value| > 10){device__protocol_services_supported$createObject =			bitstring_pss$value[10];}
        if(|bitstring_pss$value| > 11){device__protocol_services_supported$deleteObject =			bitstring_pss$value[11];}
        if(|bitstring_pss$value| > 12){device__protocol_services_supported$readProperty =			bitstring_pss$value[12];}
        if(|bitstring_pss$value| > 13){device__protocol_services_supported$readPropertyConditional =		bitstring_pss$value[13];}
        if(|bitstring_pss$value| > 14){device__protocol_services_supported$readPropertyMultiple =		bitstring_pss$value[14];}
        if(|bitstring_pss$value| > 15){device__protocol_services_supported$writeProperty =			bitstring_pss$value[15];}
        if(|bitstring_pss$value| > 16){device__protocol_services_supported$writePropertyMultiple =		bitstring_pss$value[16];}
        if(|bitstring_pss$value| > 17){device__protocol_services_supported$deviceCommunicationControl =		bitstring_pss$value[17];}
        if(|bitstring_pss$value| > 18){device__protocol_services_supported$confirmedPrivateTransfer =		bitstring_pss$value[18];}
        if(|bitstring_pss$value| > 19){device__protocol_services_supported$confirmedTextMessage =		bitstring_pss$value[19];}
        if(|bitstring_pss$value| > 20){device__protocol_services_supported$reinitializeDevice =			bitstring_pss$value[20];}
        if(|bitstring_pss$value| > 21){device__protocol_services_supported$vtOpen =				bitstring_pss$value[21];}
        if(|bitstring_pss$value| > 22){device__protocol_services_supported$vtClose =				bitstring_pss$value[22];}
        if(|bitstring_pss$value| > 23){device__protocol_services_supported$vtData =				bitstring_pss$value[23];}
        if(|bitstring_pss$value| > 24){device__protocol_services_supported$authenticate =			bitstring_pss$value[24];}
        if(|bitstring_pss$value| > 25){device__protocol_services_supported$requestKey =				bitstring_pss$value[25];}
        if(|bitstring_pss$value| > 26){device__protocol_services_supported$i_Am =				bitstring_pss$value[26];}
        if(|bitstring_pss$value| > 27){device__protocol_services_supported$i_Have =				bitstring_pss$value[27];}
        if(|bitstring_pss$value| > 28){device__protocol_services_supported$unconfirmedCOVNotification =		bitstring_pss$value[28];}
        if(|bitstring_pss$value| > 29){device__protocol_services_supported$unconfirmedEventNotification =	bitstring_pss$value[29];}
        if(|bitstring_pss$value| > 30){device__protocol_services_supported$unconfirmedPrivateTransfer =		bitstring_pss$value[30];}
        if(|bitstring_pss$value| > 31){device__protocol_services_supported$unconfirmedTextMessage =		bitstring_pss$value[31];}
        if(|bitstring_pss$value| > 32){device__protocol_services_supported$timeSynchronization =		bitstring_pss$value[32];}
        if(|bitstring_pss$value| > 33){device__protocol_services_supported$who_Has =				bitstring_pss$value[33];}
        if(|bitstring_pss$value| > 34){device__protocol_services_supported$who_Is =				bitstring_pss$value[34];}
        if(|bitstring_pss$value| > 35){device__protocol_services_supported$readRange =				bitstring_pss$value[35];}
        if(|bitstring_pss$value| > 36){device__protocol_services_supported$utcTimeSynchronization =		bitstring_pss$value[36];}
        if(|bitstring_pss$value| > 37){device__protocol_services_supported$lifeSafetyOperation =		bitstring_pss$value[37];}
        if(|bitstring_pss$value| > 38){device__protocol_services_supported$subscribeCOVProperty =		bitstring_pss$value[38];}
        if(|bitstring_pss$value| > 39){device__protocol_services_supported$getEventInformation =		bitstring_pss$value[39];}
        if(|bitstring_pss$value| > 40){device__protocol_services_supported$writeGroup =				bitstring_pss$value[40];}
        return device__protocol_services_supported;
 
      case "device/protocol-object-types-supported":
        local bitstring_pots : BitString = resources[index]$value;
        local device__protocol_object_types_supported : BACnetAPDUTypes::BACnetObjectTypesSupported;
        if(|bitstring_pots$value| > 0){device__protocol_object_types_supported$analog_input = 			bitstring_pots$value[0];}
        if(|bitstring_pots$value| > 1){device__protocol_object_types_supported$analog_output =			bitstring_pots$value[1];}
        if(|bitstring_pots$value| > 2){device__protocol_object_types_supported$analog_value =			bitstring_pots$value[2];}
        if(|bitstring_pots$value| > 3){device__protocol_object_types_supported$binary_input =			bitstring_pots$value[3];}
        if(|bitstring_pots$value| > 4){device__protocol_object_types_supported$binary_output =			bitstring_pots$value[4];}
        if(|bitstring_pots$value| > 5){device__protocol_object_types_supported$binary_value =			bitstring_pots$value[5];}
        if(|bitstring_pots$value| > 6){device__protocol_object_types_supported$calendar =			bitstring_pots$value[6];}
        if(|bitstring_pots$value| > 7){device__protocol_object_types_supported$command =			bitstring_pots$value[7];}
        if(|bitstring_pots$value| > 8){device__protocol_object_types_supported$device =				bitstring_pots$value[8];}
        if(|bitstring_pots$value| > 9){device__protocol_object_types_supported$event_enrollment =		bitstring_pots$value[9];}
        if(|bitstring_pots$value| > 10){device__protocol_object_types_supported$file_ =				bitstring_pots$value[10];}
        if(|bitstring_pots$value| > 11){device__protocol_object_types_supported$group =				bitstring_pots$value[11];}
        if(|bitstring_pots$value| > 12){device__protocol_object_types_supported$loop =				bitstring_pots$value[12];}
        if(|bitstring_pots$value| > 13){device__protocol_object_types_supported$multi_state_input =		bitstring_pots$value[13];}
        if(|bitstring_pots$value| > 14){device__protocol_object_types_supported$multi_state_output =		bitstring_pots$value[14];}
        if(|bitstring_pots$value| > 15){device__protocol_object_types_supported$notification_class =		bitstring_pots$value[15];}
        if(|bitstring_pots$value| > 16){device__protocol_object_types_supported$program =			bitstring_pots$value[16];}
        if(|bitstring_pots$value| > 17){device__protocol_object_types_supported$schedule_ =			bitstring_pots$value[17];}
        if(|bitstring_pots$value| > 18){device__protocol_object_types_supported$averaging =			bitstring_pots$value[18];}
        if(|bitstring_pots$value| > 19){device__protocol_object_types_supported$multi_state_value =		bitstring_pots$value[19];}
        if(|bitstring_pots$value| > 20){device__protocol_object_types_supported$trend_log =			bitstring_pots$value[20];}
        if(|bitstring_pots$value| > 21){device__protocol_object_types_supported$life_safety_point =		bitstring_pots$value[21];}
        if(|bitstring_pots$value| > 22){device__protocol_object_types_supported$life_safety_zone =		bitstring_pots$value[22];}
        if(|bitstring_pots$value| > 23){device__protocol_object_types_supported$accumulator =			bitstring_pots$value[23];}
        if(|bitstring_pots$value| > 24){device__protocol_object_types_supported$pulse_converter =		bitstring_pots$value[24];}
        if(|bitstring_pots$value| > 25){device__protocol_object_types_supported$event_log =			bitstring_pots$value[25];}
        if(|bitstring_pots$value| > 26){device__protocol_object_types_supported$global_group =			bitstring_pots$value[26];}
        if(|bitstring_pots$value| > 27){device__protocol_object_types_supported$trend_log_multiple =		bitstring_pots$value[27];}
        if(|bitstring_pots$value| > 28){device__protocol_object_types_supported$load_control =			bitstring_pots$value[28];}
        if(|bitstring_pots$value| > 29){device__protocol_object_types_supported$structured_view =		bitstring_pots$value[29];}
        if(|bitstring_pots$value| > 30){device__protocol_object_types_supported$access_door =			bitstring_pots$value[30];}
        if(|bitstring_pots$value| > 31){device__protocol_object_types_supported$access_credential =		bitstring_pots$value[31];}
        if(|bitstring_pots$value| > 32){device__protocol_object_types_supported$access_point =			bitstring_pots$value[32];}
        if(|bitstring_pots$value| > 33){device__protocol_object_types_supported$access_rights =			bitstring_pots$value[33];}
        if(|bitstring_pots$value| > 34){device__protocol_object_types_supported$access_user =			bitstring_pots$value[34];}
        if(|bitstring_pots$value| > 35){device__protocol_object_types_supported$access_zone =			bitstring_pots$value[35];}
        if(|bitstring_pots$value| > 36){device__protocol_object_types_supported$credential_data_input =		bitstring_pots$value[36];}
        if(|bitstring_pots$value| > 37){device__protocol_object_types_supported$network_security =		bitstring_pots$value[37];}
        if(|bitstring_pots$value| > 38){device__protocol_object_types_supported$bitstring_value =		bitstring_pots$value[38];}
        if(|bitstring_pots$value| > 39){device__protocol_object_types_supported$characterstring_value =		bitstring_pots$value[39];}
        if(|bitstring_pots$value| > 40){device__protocol_object_types_supported$date_pattern_value =		bitstring_pots$value[40];}
        if(|bitstring_pots$value| > 41){device__protocol_object_types_supported$date_value =			bitstring_pots$value[41];}
        if(|bitstring_pots$value| > 42){device__protocol_object_types_supported$datetime_pattern_value =	bitstring_pots$value[42];}
        if(|bitstring_pots$value| > 43){device__protocol_object_types_supported$datetime_value =		bitstring_pots$value[43];}
        if(|bitstring_pots$value| > 44){device__protocol_object_types_supported$integer_value =			bitstring_pots$value[44];}
        if(|bitstring_pots$value| > 45){device__protocol_object_types_supported$large_analog_value =		bitstring_pots$value[45];}
        if(|bitstring_pots$value| > 46){device__protocol_object_types_supported$octetstring_value =		bitstring_pots$value[46];}
        if(|bitstring_pots$value| > 47){device__protocol_object_types_supported$positive_integer_value =	bitstring_pots$value[47];}
        if(|bitstring_pots$value| > 48){device__protocol_object_types_supported$time_pattern_value =		bitstring_pots$value[48];}
        if(|bitstring_pots$value| > 49){device__protocol_object_types_supported$time_value =			bitstring_pots$value[49];}
        if(|bitstring_pots$value| > 50){device__protocol_object_types_supported$notification_forwarder =	bitstring_pots$value[50];}
        if(|bitstring_pots$value| > 51){device__protocol_object_types_supported$alert_enrollment =		bitstring_pots$value[51];}
        if(|bitstring_pots$value| > 52){device__protocol_object_types_supported$channel =			bitstring_pots$value[52];}
        if(|bitstring_pots$value| > 53){device__protocol_object_types_supported$lighting_output =		bitstring_pots$value[53];}
        return device__protocol_object_types_supported;

      case "device/segmentation-supported":
        local device__segmentation_supported : Enumerated = resources[index]$value;
        return BACnetAPDUTypes::BACnetSegmentation[device__segmentation_supported$value];

      default:
	 local value: Any = resources[index]$value;
	 values[|values|] = value$value;
         break;

    }

    index = index + 1;

  }

  if(|values| == 1){ 
    return values[0];
  } else {  
   return values;  
  }

}

#################################################################
#								#
#		        BACNET EVENTS				#
#								#
#################################################################

export {

  global bacnet_InformationUnit: event(c: connection, message: string, object: BACnetAPDUTypes::BACnetObjectIdentifier &default = [$id = "(no object present)", $instance = 0], property: string &default = "(no property present)", value: any &default = "(no value present)");	# FIXME: This may carry a value with the label to help the parsing

}

function triggerInformationUnit(c: connection, message: string, object: BACnetAPDUTypes::BACnetObjectIdentifier &default = [$id = "(no object present)", $instance = 0], property: string &default = "(no property present)", value: any &default = "(no value present)")
{
  
  Log::write(BACnetAPDU::INFORMATION_UNITS, [$timestamp = fmt("%s", network_time()), $message = message, $objectid = object$id, $objectinstance = object$instance, $property = property, $value = fmt("\"%s\"", value)]);

  event bacnet_InformationUnit(c, message, object, property, value);

}

################################################################################################

################################################################
#   event bacnet_Error(c, invokeid, Error_Info, Request_Info)  #
################################################################
export {

  type Error_Info : record {

    service:		string;
    errorClass:		string;
    errorCode:		string;

    bacnetRequestInfo:	Request_Info;

    informationUnit:	InformationUnit		&optional;				# INFO: This variable stores an information unit related to the "bacnetRequestInfo"

  };

  global bacnet_Error: event(c: connection, invokeid: count, error_Info: Error_Info);

}

event bacnet_apdu_error (c: connection, invokeid: count, service: string, errorClass: string, errorCode: string) &priority = 5
{

  local error_Info: Error_Info;
  
  error_Info$service = fmt("%s", service);
  error_Info$service = error_Info$service[37:|error_Info$service|];
  error_Info$errorClass = fmt("%s", errorClass);
  error_Info$errorClass = error_Info$errorClass[19:|error_Info$errorClass|];
  error_Info$errorCode = fmt("%s", errorCode);
  error_Info$errorCode = error_Info$errorCode[18:|error_Info$errorCode|];

  error_Info$bacnetRequestInfo = popRequestInfo(invokeid, error_Info$service);

  if(error_Info$bacnetRequestInfo?$content){
     error_Info$informationUnit = extractInformationUnit(error_Info$service, error_Info$bacnetRequestInfo$content, T);
  } else {
    error_Info$informationUnit = [$message = ""];
  }

  event bacnet_Error(c, invokeid, error_Info);

}

##########################################################
#   event bacnet_SimpleACK(c, invokeid, SimpleACK_Info)  #
##########################################################
export {

  type SimpleACK_Info : record {

    service:		string;

    bacnetRequestInfo:	Request_Info;

    informationUnit:	InformationUnit		&optional;				# INFO: This variable stores an information unit related to the "bacnetRequestInfo"

  };

  global bacnet_SimpleACK: event(c: connection, invokeid: count, simpleACK_Info: SimpleACK_Info);

}

event bacnet_apdu_simple_ack (c: connection, invokeid: count, service: string) &priority = 5
{

  local simpleACK_Info: SimpleACK_Info;
  
  simpleACK_Info$service = fmt("%s", service);
  simpleACK_Info$service = simpleACK_Info$service[37:|simpleACK_Info$service|];

  simpleACK_Info$bacnetRequestInfo = popRequestInfo(invokeid, simpleACK_Info$service);

  if(simpleACK_Info$bacnetRequestInfo?$content){
     simpleACK_Info$informationUnit = extractInformationUnit(simpleACK_Info$service, simpleACK_Info$bacnetRequestInfo$content, T);
  } else {
    simpleACK_Info$informationUnit = [$message = ""];
  }

  event bacnet_SimpleACK(c, invokeid, simpleACK_Info);

}

#################################################################
#								#
#			BACNET SERVICES				#
#								#
#################################################################

################################################################################
#   event bacnet_ReadProperty_Request(c, invokeid, readProperty_Request_Info)  #
################################################################################
export { 

  type ReadProperty_Request_Info : record {

    objectIdentifier: 	BACnetAPDUTypes::BACnetObjectIdentifier;
    propertyIdentifier:	string;
    propertyArrayIndex:	count						&optional;

  };

  global bacnet_ReadProperty_Request: event(c: connection, invokeid: count, info: ReadProperty_Request_Info);

}

event bacnet_read_property_request(c: connection, invokeid: count) &priority = 5
{

  local readProperty_Request_Info : ReadProperty_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      readProperty_Request_Info$objectIdentifier = object$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local property : Property = resources[i]$value;
      readProperty_Request_Info$propertyIdentifier = property$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){
  
      local unsigned : Unsigned = resources[i]$value;
      readProperty_Request_Info$propertyArrayIndex = unsigned$value;

    }

  }
  
  pushRequestInfo(invokeid, "readProperty", readProperty_Request_Info);

  triggerInformationUnit(c, "ReadProperty_Request", readProperty_Request_Info$objectIdentifier, readProperty_Request_Info$propertyIdentifier);
  
  event bacnet_ReadProperty_Request(c, invokeid, readProperty_Request_Info);

}

########################################################################
#   event bacnet_ReadProperty_ACK(c, invokeid, readProperty_ACK_Info)  #
########################################################################
export { 

  type ReadProperty_ACK_Info : record {

    objectIdentifier: 	BACnetAPDUTypes::BACnetObjectIdentifier;
    propertyIdentifier:	string;
    propertyArrayIndex:	count						&optional;
    propertyValue:	any;

    bacnetRequestInfo: 	ReadProperty_Request_Info			&optional;

  };  

  global bacnet_ReadProperty_ACK: event(c: connection, invokeid: count, info: ReadProperty_ACK_Info); 

}

event bacnet_read_property_ack(c: connection, invokeid: count) &priority = 5
{
  
  local readProperty_ACK_Info : ReadProperty_ACK_Info;

  for(i in resources){
    
    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      readProperty_ACK_Info$objectIdentifier = object$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local property : Property = resources[i]$value;
      readProperty_ACK_Info$propertyIdentifier = property$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){
  
      local unsigned : Unsigned = resources[i]$value;
      readProperty_ACK_Info$propertyArrayIndex = unsigned$value;

    } else if(resources[i]$tagPath[0:3] == "[3]"){

      readProperty_ACK_Info$propertyValue = parseAbstractSyntaxAndType(i, "[3]", readProperty_ACK_Info$objectIdentifier$id, readProperty_ACK_Info$propertyIdentifier);
      
    }

  }

  local request_Info: Request_Info = popRequestInfo(invokeid, "readProperty");
  if(request_Info?$content){
    readProperty_ACK_Info$bacnetRequestInfo = request_Info$content;
  }
  
  if(readProperty_ACK_Info?$propertyValue){
    triggerInformationUnit(c, "ReadProperty_ACK", readProperty_ACK_Info$objectIdentifier, readProperty_ACK_Info$propertyIdentifier, readProperty_ACK_Info$propertyValue);
  } else {
    triggerInformationUnit(c, "ReadProperty_ACK", readProperty_ACK_Info$objectIdentifier, readProperty_ACK_Info$propertyIdentifier);
  }

  event bacnet_ReadProperty_ACK(c, invokeid, readProperty_ACK_Info);

}

################################################################################################
#   event bacnet_ReadPropertyMultiple_Request(c, invokeid, readPropertyMultiple_Request_Info)  #
################################################################################################
export {

  type ReadPropertyMultiple_Request_Info : record {

    listOfReadAccessSpecs: 	vector of BACnetAPDUTypes::ReadAccessSpecification;

  };
 
  global bacnet_ReadPropertyMultiple_Request: event(c: connection, invokeid: count, info: ReadPropertyMultiple_Request_Info); 

}

event bacnet_read_property_multiple_request(c: connection, invokeid: count) &priority = 5
{

  local readPropertyMultiple_Request_Info : ReadPropertyMultiple_Request_Info;
 
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      readPropertyMultiple_Request_Info$listOfReadAccessSpecs[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs|] = [$objectIdentifier = object$value, $listOfPropertyReferences = vector()];

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[0]"){

      local property : Property = resources[i]$value;
      readPropertyMultiple_Request_Info$listOfReadAccessSpecs[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs| - 1]$listOfPropertyReferences[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs| - 1]$listOfPropertyReferences|] = [$propertyIdentifier = property$value];

      triggerInformationUnit(c, "ReadPropertyMultiple_Request", readPropertyMultiple_Request_Info$listOfReadAccessSpecs[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs| - 1]$objectIdentifier, readPropertyMultiple_Request_Info$listOfReadAccessSpecs[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs| - 1]$listOfPropertyReferences[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs| - 1]$listOfPropertyReferences| - 1]$propertyIdentifier);

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[1]"){

      local unsigned : Unsigned = resources[i]$value;
      readPropertyMultiple_Request_Info$listOfReadAccessSpecs[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs| - 1]$listOfPropertyReferences[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs[|readPropertyMultiple_Request_Info$listOfReadAccessSpecs| - 1]$listOfPropertyReferences| - 1]$propertyArrayIndex = unsigned$value;

    }

  }

  pushRequestInfo(invokeid, "readPropertyMultiple", readPropertyMultiple_Request_Info);

  event bacnet_ReadPropertyMultiple_Request(c, invokeid, readPropertyMultiple_Request_Info);

}

########################################################################################
#   event bacnet_ReadPropertyMultiple_ACK(c, invokeid, readPropertyMultiple_ACK_Info)  #
########################################################################################
export {

  type ReadPropertyMultiple_ACK_Info : record {

    listOfReadAccessResults: 	vector of BACnetAPDUTypes::ReadAccessResult;

    bacnetRequestInfo: 		ReadPropertyMultiple_Request_Info				&optional;

  };

  global bacnet_ReadPropertyMultiple_ACK: event(c: connection, invokeid: count, info: ReadPropertyMultiple_ACK_Info);

}

event bacnet_read_property_multiple_ack(c: connection, invokeid: count) &priority = 5
{
  
  local readPropertyMultiple_ACK_Info : ReadPropertyMultiple_ACK_Info;

  local errorFlag = F;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults|] = [$objectIdentifier = object$value, $listOfResults = vector()];

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[2]"){

      local property : Property = resources[i]$value;
      readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults|] = [$propertyIdentifier = property$value];

      errorFlag = F;

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[3]"){

      local unsigned : Unsigned = resources[i]$value;
      readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults| - 1]$propertyArrayIndex = unsigned$value;

    } else if(resources[i]$tagPath[0:6] == "[1][4]"){

      readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults| - 1]$propertyValue = parseAbstractSyntaxAndType(i, "[1][4]", readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$objectIdentifier$id, readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults| - 1]$propertyIdentifier);

      triggerInformationUnit(c, "ReadPropertyMultiple_ACK", readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$objectIdentifier, readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults| - 1]$propertyIdentifier, readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults| - 1]$propertyValue);		# INFO: This has to be triggered only if a value is returned

    } else if(resources[i]$tagPath[0:6] == "[1][5]" && errorFlag == F){

      local error : BACnetAPDUTypes::Error = parseError(i, "[1][5]");
      readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults| - 1]$propertyAccessError = error;

      errorFlag = T;

      event bacnet_Error(c, invokeid, [$service = "readPropertyMultiple_(property)", $errorClass = error$error_class, $errorCode = error$error_code, $bacnetRequestInfo = [$service = "readPropertyMultiple_(property)", $content = [$object = readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$objectIdentifier, $property = readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults| - 1]$propertyIdentifier]], $informationUnit = [$message = "ReadPropertyMultiple_ACK_(property)", $object = readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$objectIdentifier, $property = readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults[|readPropertyMultiple_ACK_Info$listOfReadAccessResults| - 1]$listOfResults| - 1]$propertyIdentifier]]);	# INFO: This is an error included in a readPropertyMultiple	# FIXME: Not sure if calling the standard bacnetError event is the best way to deal with this situation

    }

  }

  local request_Info: Request_Info = popRequestInfo(invokeid, "readPropertyMultiple");
  if(request_Info?$content){
    readPropertyMultiple_ACK_Info$bacnetRequestInfo = request_Info$content;
  }

  event bacnet_ReadPropertyMultiple_ACK(c, invokeid, readPropertyMultiple_ACK_Info);

}

##########################################################################
#   event bacnet_ReadRange_Request(c, invokeid, readRange_Request_Info)  #
##########################################################################
export { 

  type ReadRange_Request_Info : record {

    objectIdentifier: 	BACnetAPDUTypes::BACnetObjectIdentifier;
    propertyIdentifier:	string;
    propertyArrayIndex:	count						&optional;
    range:		any						&optional;

  };

  global bacnet_ReadRange_Request: event(c: connection, invokeid: count, info: ReadRange_Request_Info);

}

event bacnet_read_range_request(c: connection, invokeid: count) &priority = 5
{

  local readRange_Request_Info : ReadRange_Request_Info;

  local rangeFlag = F;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      readRange_Request_Info$objectIdentifier = object$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local property : Property = resources[i]$value;
      readRange_Request_Info$propertyIdentifier = property$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){
  
      local unsigned : Unsigned = resources[i]$value;
      readRange_Request_Info$propertyArrayIndex = unsigned$value;

    } else if(resources[i]$tagPath[0:3] == "[3]" && rangeFlag == F){
  
      local referenceIndex : Unsigned = resources[i]$value;
      local count_ : Integer = resources[i + 1]$value;
      readRange_Request_Info$range = [$referenceIndex = referenceIndex$value, $count_ = count_$value];

      i = i + 1;
      rangeFlag = T;

    } else if(resources[i]$tagPath[0:3] == "[6]" && rangeFlag == F){
  
      local referenceSequenceNumber : Unsigned = resources[i]$value;
      local count2_ : Integer = resources[i + 1]$value;
      readRange_Request_Info$range = [$referenceSequenceNumber = referenceSequenceNumber$value, $count_ = count2_$value];

      i = i + 1;
      rangeFlag = T;

    } else if(resources[i]$tagPath[0:3] == "[7]" && rangeFlag == F){
      
      local referenceTime : BACnetAPDUTypes::BACnetDateTime = parseBACnetDateTime(i, "[7]");
      local count3_ : Integer = resources[i + 1]$value;
      readRange_Request_Info$range = [$referenceTime = referenceTime, $count_ = count3_$value];

      i = i + 1;
      rangeFlag = T;

    }

  }

  pushRequestInfo(invokeid, "readRange", readRange_Request_Info);
  
  triggerInformationUnit(c, "ReadRange_Request", readRange_Request_Info$objectIdentifier, readRange_Request_Info$propertyIdentifier);
  
  event bacnet_ReadRange_Request(c, invokeid, readRange_Request_Info);

}

##################################################################
#   event bacnet_ReadRange_ACK(c, invokeid, readRange_ACK_Info)  #
##################################################################
export { 

  type ReadRange_ACK_Info : record {

    objectIdentifier: 		BACnetAPDUTypes::BACnetObjectIdentifier;
    propertyIdentifier:		string;
    propertyArrayIndex:		count							&optional;
    resultFlags:		BACnetAPDUTypes::BACnetResultFlags;
    itemCount:			count;
    itemData:			any;
    firstSequenceNumber:	count							&optional;

    bacnetRequestInfo:		ReadRange_Request_Info					&optional;

  };

  global bacnet_ReadRange_ACK: event(c: connection, invokeid: count, info: ReadRange_ACK_Info);

}

event bacnet_read_range_ack(c: connection, invokeid: count) &priority = 5
{
  
  local readRange_ACK_Info : ReadRange_ACK_Info;

  local itemDataFlag = F;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      readRange_ACK_Info$objectIdentifier = object$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local property : Property = resources[i]$value;
      readRange_ACK_Info$propertyIdentifier = property$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){
  
      local pai : Unsigned = resources[i]$value;
      readRange_ACK_Info$propertyArrayIndex = pai$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[3]"){
  
      local bitstring : BitString = resources[i]$value;
      readRange_ACK_Info$resultFlags$first_item = bitstring$value[0];
      readRange_ACK_Info$resultFlags$last_item = bitstring$value[1];
      readRange_ACK_Info$resultFlags$more_item = bitstring$value[2];

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[4]"){
  
      local ic : Unsigned = resources[i]$value;
      readRange_ACK_Info$itemCount = ic$value;

    } else if(resources[i]$tagPath[0:3] == "[5]" && itemDataFlag == F){
  
      readRange_ACK_Info$itemData = parseAbstractSyntaxAndType(i, "[5]");

      itemDataFlag = T;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[6]"){
  
      local fsn : Unsigned = resources[i]$value;
      readRange_ACK_Info$firstSequenceNumber = fsn$value;

    }

  }

  local request_Info: Request_Info = popRequestInfo(invokeid, "readRange");
  if(request_Info?$content){
    readRange_ACK_Info$bacnetRequestInfo = request_Info$content;
  }

  triggerInformationUnit(c, "ReadRange_ACK", readRange_ACK_Info$objectIdentifier, readRange_ACK_Info$propertyIdentifier);
  
  event bacnet_ReadRange_ACK(c, invokeid, readRange_ACK_Info);

}

####################################################################################
#   event bacnet_AtomicReadFile_Request(c, invokeid, atomicReadFile_Request_Info)  #
####################################################################################
export { 

  type AtomicReadFile_Request_Info : record {

    fileIdentifier: 	BACnetAPDUTypes::BACnetObjectIdentifier;
    accessMethod:	any;

  };

  global bacnet_AtomicReadFile_Request: event(c: connection, invokeid: count, info: AtomicReadFile_Request_Info);

}

event bacnet_atomic_read_file_request(c: connection, invokeid: count) &priority = 5
{

  local atomicReadFile_Request_Info : AtomicReadFile_Request_Info;

  local accessMethodFlag = F;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "BACnetObjectIdentifier"){

      local object : Object = resources[i]$value;
      atomicReadFile_Request_Info$fileIdentifier = object$value;

    } else if(resources[i]$tagPath[0:3] == "[0]" && accessMethodFlag == F){

      local fileStartPosition : Integer = resources[i]$value;
      local requestedOctetCount: Unsigned = resources[i + 1]$value;
      atomicReadFile_Request_Info$accessMethod = [$fileStartPosition = fileStartPosition$value, $requestedOctetCount = requestedOctetCount$value];

      accessMethodFlag = T;

    } else if(resources[i]$tagPath[0:3] == "[1]" && accessMethodFlag == F){
  
      local fileStartRecord : Integer = resources[i]$value;
      local requestedRecordCount: Unsigned = resources[i + 1]$value;
      atomicReadFile_Request_Info$accessMethod = [$fileStartPosition = fileStartRecord$value, $requestedOctetCount = requestedRecordCount$value];

      accessMethodFlag = T;

    }

  }

  pushRequestInfo(invokeid, "atomicReadFile", atomicReadFile_Request_Info);
   
  triggerInformationUnit(c, "AtomicReadFile_Request", atomicReadFile_Request_Info$fileIdentifier);
  
  event bacnet_AtomicReadFile_Request(c, invokeid, atomicReadFile_Request_Info);

}

############################################################################
#   event bacnet_AtomicReadFile_ACK(c, invokeid, atomicReadFile_ACK_Info)  #
############################################################################
export { 

  type AtomicReadFile_ACK_Info : record {

    endOfFile: 		bool;
    accessMethod:	any;

    bacnetRequestInfo:	AtomicReadFile_Request_Info		&optional;

  };

  global bacnet_AtomicReadFile_ACK: event(c: connection, invokeid: count, info: AtomicReadFile_ACK_Info);

}

event bacnet_atomic_read_file_ack(c: connection, invokeid: count) &priority = 5
{

  local atomicReadFile_ACK_Info : AtomicReadFile_ACK_Info;

  local accessMethodFlag = F;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "BOOLEAN"){

      local b : Boolean = resources[i]$value;
      atomicReadFile_ACK_Info$endOfFile = b$value;

    } else if(resources[i]$tagPath[0:3] == "[0]" && accessMethodFlag == F){

      local fileStartPosition : Integer = resources[i]$value;
      local fileData: Octets = resources[i + 1]$value;
      atomicReadFile_ACK_Info$accessMethod = [$fileStartPosition = fileStartPosition$value, $fileData = fileData$value];

      accessMethodFlag = T;

    } else if(resources[i]$tagPath[0:3] == "[1]" && accessMethodFlag == F){
  
      local fileStartRecord : Integer = resources[i]$value;
      local returnedRecordCount: Unsigned = resources[i + 1]$value;
      local fileRecordData: Octets = resources[i + 2]$value;
      atomicReadFile_ACK_Info$accessMethod = [$fileStartRecord = fileStartRecord$value, $returnedRecordCount = returnedRecordCount$value, $fileRecordData = fileRecordData$value];

      accessMethodFlag = T;

    }

  }

  local request_Info: Request_Info = popRequestInfo(invokeid, "atomicReadFile");
  if(request_Info?$content){
    atomicReadFile_ACK_Info$bacnetRequestInfo = request_Info$content;
  }

  triggerInformationUnit(c, "AtomicReadFile_ACK");
  
  event bacnet_AtomicReadFile_ACK(c, invokeid, atomicReadFile_ACK_Info);

}

##########################################################################
#  event bacnet_SubscribeCOV_Request(c, invokeid, subscribeCOV_Request)  #
##########################################################################
export {

  type SubscribeCOV_Request_Info : record {

    subscriberProcessIdentifier:	count;
    monitoredObjectIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    issueConfirmedNotifications:	bool							&optional;
    lifetime:				count							&optional;

  };

  global bacnet_SubscribeCOV_Request: event(c: connection, invokeid: count, info: SubscribeCOV_Request_Info);

}

event bacnet_subscribe_cov_request(c: connection, invokeid: count) &priority = 5
{

  local subscribeCOV_Request_Info: SubscribeCOV_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local spi : Unsigned = resources[i]$value;
      subscribeCOV_Request_Info$subscriberProcessIdentifier = spi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local moi : Object = resources[i]$value;
      subscribeCOV_Request_Info$monitoredObjectIdentifier = moi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){

      local icn : Boolean = resources[i]$value;
      subscribeCOV_Request_Info$issueConfirmedNotifications = icn$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[3]"){

      local l : Unsigned = resources[i]$value;
      subscribeCOV_Request_Info$lifetime = l$value;

    }

  }

  pushRequestInfo(invokeid, "subscribeCOV", subscribeCOV_Request_Info);
  
  triggerInformationUnit(c, "SubscribeCOV_Request", subscribeCOV_Request_Info$monitoredObjectIdentifier);
  
  event bacnet_SubscribeCOV_Request(c, invokeid, subscribeCOV_Request_Info);

}

##########################################################################################
#  event bacnet_SubscribeCOVProperty_Request(c, invokeid, subscribeCOVProperty_Request)  #
##########################################################################################
export {

  type SubscribeCOVProperty_Request_Info : record {

    subscriberProcessIdentifier:	count;
    monitoredObjectIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    issueConfirmedNotifications:	bool							&optional;
    lifetime:				count							&optional;
    monitoredPropertyIdentifier:	BACnetAPDUTypes::BACnetPropertyReference;
    covIncrement:			double							&optional;

  };

  global bacnet_SubscribeCOVProperty_Request: event(c: connection, invokeid: count, info: SubscribeCOVProperty_Request_Info);

}

event bacnet_subscribe_cov_property_request(c: connection, invokeid: count) &priority = 5
{

  local subscribeCOVProperty_Request_Info: SubscribeCOVProperty_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local spi : Unsigned = resources[i]$value;
      subscribeCOVProperty_Request_Info$subscriberProcessIdentifier = spi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local moi : Object = resources[i]$value;
      subscribeCOVProperty_Request_Info$monitoredObjectIdentifier = moi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){

      local icn : Boolean = resources[i]$value;
      subscribeCOVProperty_Request_Info$issueConfirmedNotifications = icn$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[3]"){

      local l : Unsigned = resources[i]$value;
      subscribeCOVProperty_Request_Info$lifetime = l$value;

    } else if(resources[i]$tagPath == "[4]" && resources[i]$tagLabel == "[0]"){

      local property : Property = resources[i]$value;
      subscribeCOVProperty_Request_Info$monitoredPropertyIdentifier$propertyIdentifier = property$value;

    } else if(resources[i]$tagPath == "[4]" && resources[i]$tagLabel == "[1]"){
  
      local unsigned : Unsigned = resources[i]$value;
      subscribeCOVProperty_Request_Info$monitoredPropertyIdentifier$propertyArrayIndex = unsigned$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[5]"){

      local ci : Real = resources[i]$value;
      subscribeCOVProperty_Request_Info$covIncrement = ci$value;

    }

  }

  pushRequestInfo(invokeid, "subscribeCOVProperty", subscribeCOVProperty_Request_Info);
  
  triggerInformationUnit(c, "SubscribeCOVProperty_Request", subscribeCOVProperty_Request_Info$monitoredObjectIdentifier, subscribeCOVProperty_Request_Info$monitoredPropertyIdentifier$propertyIdentifier);
  
  event bacnet_SubscribeCOVProperty_Request(c, invokeid, subscribeCOVProperty_Request_Info);

}

##################################################################################################
#  event bacnet_ConfirmedCOVNotification_Request(c, invokeid, confirmedCOVNotification_Request)  #
##################################################################################################
export {

  type ConfirmedCOVNotification_Request_Info : record {

    subscriberProcessIdentifier:	count;
    initiatingDeviceIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    monitoredObjectIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    timeRemaining:			count;
    listOfValues:			vector of BACnetAPDUTypes::BACnetPropertyValue;

  };

  global bacnet_ConfirmedCOVNotification_Request: event(c: connection, invokeid: count, info: ConfirmedCOVNotification_Request_Info);

}

event bacnet_confirmed_cov_notification_request(c: connection, invokeid: count) &priority = 5
{

  local confirmedCOVNotification_Request_Info: ConfirmedCOVNotification_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local spi : Unsigned = resources[i]$value;
      confirmedCOVNotification_Request_Info$subscriberProcessIdentifier = spi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local idi : Object = resources[i]$value;
      confirmedCOVNotification_Request_Info$initiatingDeviceIdentifier = idi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){

      local moi : Object = resources[i]$value;
      confirmedCOVNotification_Request_Info$monitoredObjectIdentifier = moi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[3]"){

      local tr : Unsigned = resources[i]$value;
      confirmedCOVNotification_Request_Info$timeRemaining = tr$value;

    } else if(resources[i]$tagPath == "[4]" && resources[i]$tagLabel == "[0]"){

      local property : Property = resources[i]$value;
      confirmedCOVNotification_Request_Info$listOfValues[|confirmedCOVNotification_Request_Info$listOfValues|] = [$propertyIdentifier = property$value, $value = ""];

    } else if(resources[i]$tagPath == "[4]" && resources[i]$tagLabel == "[1]"){

      local pai : Unsigned = resources[i]$value;
      confirmedCOVNotification_Request_Info$listOfValues[|confirmedCOVNotification_Request_Info$listOfValues| - 1]$propertyArrayIndex = pai$value;

    } else if(resources[i]$tagPath[0:6] == "[4][2]"){

      confirmedCOVNotification_Request_Info$listOfValues[|confirmedCOVNotification_Request_Info$listOfValues| - 1]$value = parseAbstractSyntaxAndType(i, "[4][2]", confirmedCOVNotification_Request_Info$monitoredObjectIdentifier$id, confirmedCOVNotification_Request_Info$listOfValues[|confirmedCOVNotification_Request_Info$listOfValues| - 1]$propertyIdentifier);

      triggerInformationUnit(c, "ConfirmedCOVNotification_Request", confirmedCOVNotification_Request_Info$monitoredObjectIdentifier, confirmedCOVNotification_Request_Info$listOfValues[|confirmedCOVNotification_Request_Info$listOfValues| - 1]$propertyIdentifier, confirmedCOVNotification_Request_Info$listOfValues[|confirmedCOVNotification_Request_Info$listOfValues| - 1]$value);

    } else if(resources[i]$tagPath == "[4]" && resources[i]$tagLabel == "[3]"){

      local p : Unsigned = resources[i]$value;
      confirmedCOVNotification_Request_Info$listOfValues[|confirmedCOVNotification_Request_Info$listOfValues| - 1]$priority = p$value;

    }

  }

  pushRequestInfo(invokeid, "confirmedCOVNotification", confirmedCOVNotification_Request_Info);
  
  event bacnet_ConfirmedCOVNotification_Request(c, invokeid, confirmedCOVNotification_Request_Info);

}

######################################################################################################
#  event bacnet_ConfirmedEventNotification_Request(c, invokeid, confirmedEventNotification_Request)  #
######################################################################################################
export {

  type ConfirmedEventNotification_Request_Info : record {

    processIdentifier:			count;
    initiatingDeviceIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    eventObjectIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    timeStamp:				any;
    notificationClass:			count;
    priority:				count;
    eventType:				string;
    messageText:			string 						&optional;
    notifyType:				string;
    ackRequired:			bool						&optional;
    fromState:				string				 		&optional;
    toState:				string;
    eventValues:			any 						&optional;

  };

  global bacnet_ConfirmedEventNotification_Request: event(c: connection, invokeid: count, info: ConfirmedEventNotification_Request_Info);

}

event bacnet_confirmed_event_notification_request(c: connection, invokeid: count) &priority = 5
{

  local confirmedEventNotification_Request_Info: ConfirmedEventNotification_Request_Info;

  local timeStampFlag: bool = F;
  local eventValuesFlag: bool = F;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local pi : Unsigned = resources[i]$value;
      confirmedEventNotification_Request_Info$processIdentifier = pi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local idi : Object = resources[i]$value;
      confirmedEventNotification_Request_Info$initiatingDeviceIdentifier = idi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){

      local eoi : Object = resources[i]$value;
      confirmedEventNotification_Request_Info$eventObjectIdentifier = eoi$value;

    } else if(resources[i]$tagPath[0:3] == "[3]" && timeStampFlag == F){

      local t: any = parseBACnetTimeStamps(i, "[3]", 1);
      confirmedEventNotification_Request_Info$timeStamp = t;

      timeStampFlag = T;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[4]"){

      local nc : Unsigned = resources[i]$value;
      confirmedEventNotification_Request_Info$notificationClass = nc$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[5]"){

      local p : Unsigned = resources[i]$value;
      confirmedEventNotification_Request_Info$priority = p$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[6]"){

      local et : Unsigned = resources[i]$value;
      confirmedEventNotification_Request_Info$eventType = BACnetAPDUTypes::BACnetEventType[et$value];

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[7]"){

      local mt : CharacterString = resources[i]$value;
      confirmedEventNotification_Request_Info$messageText = mt$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[8]"){

      local nt : Unsigned = resources[i]$value;
      confirmedEventNotification_Request_Info$notifyType = BACnetAPDUTypes::BACnetNotifyType[nt$value];

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[9]"){

      local ar : Boolean = resources[i]$value;
      confirmedEventNotification_Request_Info$ackRequired = ar$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[10]"){

      local fs : Unsigned = resources[i]$value;
      confirmedEventNotification_Request_Info$fromState = BACnetAPDUTypes::BACnetEventState[fs$value];

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[11]"){

      local ts : Unsigned = resources[i]$value;
      confirmedEventNotification_Request_Info$toState = BACnetAPDUTypes::BACnetEventState[ts$value];

    } else if(resources[i]$tagPath[0:4] == "[12]" && eventValuesFlag == F){

      local ev : any = parseBACnetNotificationParameters(i, "[12]");
      confirmedEventNotification_Request_Info$eventValues = ev;

      eventValuesFlag = T;

    }

  }

  pushRequestInfo(invokeid, "confirmedEventNotification", confirmedEventNotification_Request_Info);
  
  triggerInformationUnit(c, "ConfirmedEventNotification_Request", confirmedEventNotification_Request_Info$eventObjectIdentifier);
  
  event bacnet_ConfirmedEventNotification_Request(c, invokeid, confirmedEventNotification_Request_Info);

}

##############################################################################################
#   event bacnet_ConfirmedPrivateTransfer_Request(c, confirmedPrivateTransfer_Request_Info)  #
##############################################################################################
export {

  type ConfirmedPrivateTransfer_Request_Info : record {

    vendorID:		string;
    serviceNumber:	count;
    serviceParameters:	any		&optional;

  };

  global bacnet_ConfirmedPrivateTransfer_Request: event(c: connection, invokeid: count, confirmedPrivateTransfer_Request_Info: ConfirmedPrivateTransfer_Request_Info);

}

event bacnet_confirmed_private_transfer_request(c: connection, invokeid: count) &priority = 5
{

  local confirmedPrivateTransfer_Request_Info: ConfirmedPrivateTransfer_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local vId : Unsigned = resources[i]$value;
      if(vId$value in BACnetAPDUTypes::BACnetVendorId){
        confirmedPrivateTransfer_Request_Info$vendorID = BACnetAPDUTypes::BACnetVendorId[vId$value];
      } else {
        confirmedPrivateTransfer_Request_Info$vendorID = fmt("Unknown Vendor (%s)", vId$value);
      }

   } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local sn : Unsigned = resources[i]$value;
      confirmedPrivateTransfer_Request_Info$serviceNumber = sn$value;

   } else if (resources[i]$tagPath[0:3] == "[2]"){

      confirmedPrivateTransfer_Request_Info$serviceParameters = parseAbstractSyntaxAndType(i, "[2]");

   }

  }

  pushRequestInfo(invokeid, "confirmedPrivateTransfer", confirmedPrivateTransfer_Request_Info);
  
  triggerInformationUnit(c, "ConfirmedPrivateTransfer_Request");
  
  event bacnet_ConfirmedPrivateTransfer_Request(c, invokeid, confirmedPrivateTransfer_Request_Info);

}

######################################################################################
#   event bacnet_ConfirmedPrivateTransfer_ACK(c, confirmedPrivateTransfer_ACK_Info)  #
######################################################################################
export {

  type ConfirmedPrivateTransfer_ACK_Info : record {

    vendorID:		string;
    serviceNumber:	count;
    resultBlock:	any						&optional;

    bacnetRequestInfo:	ConfirmedPrivateTransfer_Request_Info		&optional;

  };

  global bacnet_ConfirmedPrivateTransfer_ACK: event(c: connection, invokeid: count, confirmedPrivateTransfer_ACK_Info: ConfirmedPrivateTransfer_ACK_Info);

}

event bacnet_confirmed_private_transfer_ack(c: connection, invokeid: count) &priority = 5
{

  local confirmedPrivateTransfer_ACK_Info: ConfirmedPrivateTransfer_ACK_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local vId : Unsigned = resources[i]$value;
      if(vId$value in BACnetAPDUTypes::BACnetVendorId){
        confirmedPrivateTransfer_ACK_Info$vendorID = BACnetAPDUTypes::BACnetVendorId[vId$value];
      } else {
        confirmedPrivateTransfer_ACK_Info$vendorID = fmt("Unknown Vendor (%s)", vId$value);
      }

   } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local sn : Unsigned = resources[i]$value;
      confirmedPrivateTransfer_ACK_Info$serviceNumber = sn$value;

   } else if (resources[i]$tagPath[0:3] == "[2]"){

      confirmedPrivateTransfer_ACK_Info$resultBlock = parseAbstractSyntaxAndType(i, "[2]");

   }

  }

  local request_Info: Request_Info = popRequestInfo(invokeid, "confirmedPrivateTransfer");
  if(request_Info?$content){
    confirmedPrivateTransfer_ACK_Info$bacnetRequestInfo = request_Info$content;
  }

  triggerInformationUnit(c, "ConfirmedPrivateTransfer_ACK");
  
  event bacnet_ConfirmedPrivateTransfer_ACK(c, invokeid, confirmedPrivateTransfer_ACK_Info);

}

##################################################################################
#   event bacnet_WriteProperty_Request(c, invokeid, writeProperty_Request_Info)  #
##################################################################################
export { 

  type WriteProperty_Request_Info : record {

    objectIdentifier: 	BACnetAPDUTypes::BACnetObjectIdentifier;
    propertyIdentifier:	string;
    propertyArrayIndex:	count						&optional;
    propertyValue:	any;
    priority:		count						&optional;

  };

  global bacnet_WriteProperty_Request: event(c: connection, invokeid: count, info: WriteProperty_Request_Info);

}

event bacnet_write_property_request(c: connection, invokeid: count) &priority = 5
{

  local writeProperty_Request_Info : WriteProperty_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      writeProperty_Request_Info$objectIdentifier = object$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local property : Property = resources[i]$value;
      writeProperty_Request_Info$propertyIdentifier = property$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){
  
      local pai : Unsigned = resources[i]$value;
      writeProperty_Request_Info$propertyArrayIndex = pai$value;

    } else if(resources[i]$tagPath[0:3] == "[3]"){
  
      writeProperty_Request_Info$propertyValue = parseAbstractSyntaxAndType(i, "[3]", writeProperty_Request_Info$objectIdentifier$id, writeProperty_Request_Info$propertyIdentifier);

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[4]"){

      local p : Unsigned = resources[i]$value;
      writeProperty_Request_Info$priority = p$value;

    }

  }

  pushRequestInfo(invokeid, "writeProperty", writeProperty_Request_Info);
  
  triggerInformationUnit(c, "WriteProperty_Request", writeProperty_Request_Info$objectIdentifier, writeProperty_Request_Info$propertyIdentifier, writeProperty_Request_Info$propertyValue);
  
  event bacnet_WriteProperty_Request(c, invokeid, writeProperty_Request_Info);

}

##################################################################################################
#   event bacnet_WritePropertyMultiple_Request(c, invokeid, writePropertyMultiple_Request_Info)  #
##################################################################################################
export { 

  type WritePropertyMultiple_Request_Info : record {

    listOfwriteAccessSpecifications: 	vector of BACnetAPDUTypes::WriteAccessSpecification;

  };

  global bacnet_WritePropertyMultiple_Request: event(c: connection, invokeid: count, info: WritePropertyMultiple_Request_Info);

}

event bacnet_write_property_multiple_request(c: connection, invokeid: count) &priority = 5
{

  local writePropertyMultiple_Request_Info : WritePropertyMultiple_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications|] = [$objectIdentifier = object$value, $listOfProperties = vector()];

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[0]"){

      local property : Property = resources[i]$value;
      writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties|] = [$propertyIdentifier = property$value, $value = ""];

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[1]"){
  
      local pai : Unsigned = resources[i]$value;
      writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties| - 1]$propertyArrayIndex = pai$value;

    } else if(resources[i]$tagPath[0:6] == "[1][2]"){
  
      writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties| - 1]$value = parseAbstractSyntaxAndType(i, "[1][2]", writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$objectIdentifier$id, writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties| - 1]$propertyIdentifier);

      triggerInformationUnit(c, "WritePropertyMultiple_Request", writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$objectIdentifier, writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties| - 1]$propertyIdentifier, writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties| - 1]$value);

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[3]"){

      local p : Unsigned = resources[i]$value;
      writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications[|writePropertyMultiple_Request_Info$listOfwriteAccessSpecifications| - 1]$listOfProperties| - 1]$priority = p$value;

    }

  }

  pushRequestInfo(invokeid, "writePropertyMultiple", writePropertyMultiple_Request_Info);

  event bacnet_WritePropertyMultiple_Request(c, invokeid, writePropertyMultiple_Request_Info);

}

######################################################################################
#   event bacnet_AtomicWriteFile_Request(c, invokeid, atomicWriteFile_Request_Info)  #
######################################################################################
export { 

  type AtomicWriteFile_Request_Info : record {

    fileIdentifier: 	BACnetAPDUTypes::BACnetObjectIdentifier;
    accessMethod:	any;

  };

  global bacnet_AtomicWriteFile_Request: event(c: connection, invokeid: count, info: AtomicWriteFile_Request_Info);

}

event bacnet_atomic_write_file_request(c: connection, invokeid: count) &priority = 5
{

  local atomicWriteFile_Request_Info : AtomicWriteFile_Request_Info;

  local accessMethodFlag = F;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "BACnetObjectIdentifier"){

      local object : Object = resources[i]$value;
      atomicWriteFile_Request_Info$fileIdentifier = object$value;

    } else if(resources[i]$tagPath[0:3] == "[0]" && accessMethodFlag == F){

      local fileStartPosition : Integer = resources[i]$value;
      local fileData: Octets = resources[i + 1]$value;
      atomicWriteFile_Request_Info$accessMethod = [$fileStartPosition = fileStartPosition$value, $fileData = fileData$value];

      accessMethodFlag = T;

    } else if(resources[i]$tagPath[0:3] == "[1]" && accessMethodFlag == F){
  
      local fileStartRecord : Integer = resources[i]$value;
      local recordCount: Unsigned = resources[i + 1]$value;
      local fileRecordData: Octets = resources[i + 2]$value;
      atomicWriteFile_Request_Info$accessMethod = [$fileStartRecord = fileStartRecord$value, $recordCount = recordCount$value, $fileRecordData = fileRecordData$value];

      accessMethodFlag = T;

    }

  }

  pushRequestInfo(invokeid, "atomicWriteFile", atomicWriteFile_Request_Info);
  
  triggerInformationUnit(c, "AtomicWriteFile_Request", atomicWriteFile_Request_Info$fileIdentifier);
  
  event bacnet_AtomicWriteFile_Request(c, invokeid, atomicWriteFile_Request_Info);

}

##############################################################################
#   event bacnet_AtomicWriteFile_ACK(c, invokeid, atomicWriteFile_ACK_Info)  #
##############################################################################
export { 

  type AtomicWriteFile_ACK_Info : record {

    fileStartPosition: 	int				&optional;
    fileStartRecord:	int				&optional;

    bacnetRequestInfo:	AtomicWriteFile_Request_Info	&optional;

  };

  global bacnet_AtomicWriteFile_ACK: event(c: connection, invokeid: count, atomicWriteFile_ACK_Info: AtomicWriteFile_ACK_Info);

}

event bacnet_atomic_write_file_ack(c: connection, invokeid: count) &priority = 5
{

  local atomicWriteFile_ACK_Info: AtomicWriteFile_ACK_Info;
  
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local fsp : Integer = resources[i]$value;
      atomicWriteFile_ACK_Info$fileStartPosition = fsp$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local fsr : Integer = resources[i]$value;
      atomicWriteFile_ACK_Info$fileStartRecord = fsr$value;

    }
  
  }

  local request_Info: Request_Info = popRequestInfo(invokeid, "atomicWriteFile");
  if(request_Info?$content){
    atomicWriteFile_ACK_Info$bacnetRequestInfo = request_Info$content;
  }
  
  triggerInformationUnit(c, "AtomicWriteFile_ACK");
  
  event bacnet_AtomicWriteFile_ACK(c, invokeid, atomicWriteFile_ACK_Info);

}

##############################################################################################
#   event bacnet_GetEventInformation_Request(c, invokeid, getEventInformation_Request_Info)  #
##############################################################################################
export { 

  type GetEventInformation_Request_Info : record {

    lastReceivedObjectIdentifier: 	BACnetAPDUTypes::BACnetObjectIdentifier 	&optional;

  };

  global bacnet_GetEventInformation_Request: event(c: connection, invokeid: count, info: GetEventInformation_Request_Info);

}

event bacnet_get_event_information_request(c: connection, invokeid: count)
{
  
  local getEventInformation_Request_Info : GetEventInformation_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      getEventInformation_Request_Info$lastReceivedObjectIdentifier = object$value;

    } 

  }

  pushRequestInfo(invokeid, "getEventInformation", getEventInformation_Request_Info);

  if(getEventInformation_Request_Info?$lastReceivedObjectIdentifier){
    triggerInformationUnit(c, "GetEventInformation_Request", getEventInformation_Request_Info$lastReceivedObjectIdentifier);
  } else{
    triggerInformationUnit(c, "GetEventInformation_Request");
  }
  
  event bacnet_GetEventInformation_Request(c, invokeid, getEventInformation_Request_Info);

}

######################################################################################
#   event bacnet_GetEventInformation_ACK(c, invokeid, getEventInformation_ACK_Info)  #
######################################################################################
export { 

  type GetEventInformation_ACK_Info : record {

    listOfEventSummaries: 	vector of record {
    					    	  objectIdentifier: 		BACnetAPDUTypes::BACnetObjectIdentifier;
    					    	  eventState:			string;
    					    	  acknowledgedTransitions:	BACnetAPDUTypes::BACnetEventTransitionBits;
    					    	  eventTimeStamps:		vector of any;
    					    	  notifyType:			string;
    					    	  eventEnable:			BACnetAPDUTypes::BACnetEventTransitionBits;
    					    	  eventPriorities:		vector of count;
                                           	 };
    moreEvents:			bool;

    bacnetRequestInfo:		GetEventInformation_Request_Info		&optional;	

  };

  global bacnet_GetEventInformation_ACK: event(c: connection, invokeid: count, info: GetEventInformation_ACK_Info);

}

event bacnet_get_event_information_ack(c: connection, invokeid: count) &priority = 5
{

  local getEventInformation_ACK_Info : GetEventInformation_ACK_Info;

  local timeStampFlag = F;
  local eventPrioritiesFlag = F;
  for(i in resources){

    if(resources[i]$tagPath == "[0]" && resources[i]$tagLabel == "[0]"){

      local object : Object = resources[i]$value;
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries|] = [$objectIdentifier = object$value, $eventState = "", $acknowledgedTransitions = [$to_offnormal = F, $to_fault = F, $to_normal = F], $eventTimeStamps = vector(), $notifyType = "", $eventEnable = [$to_offnormal = F, $to_fault = F, $to_normal = F], $eventPriorities = vector()];

      timeStampFlag = F;
      eventPrioritiesFlag = F;

      triggerInformationUnit(c, "GetEventInformation_ACK", getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$objectIdentifier);
    
    } else if(resources[i]$tagPath == "[0]" && resources[i]$tagLabel == "[1]"){

      local es : Unsigned = resources[i]$value;
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$eventState = BACnetAPDUTypes::BACnetEventState[es$value];

    } else if(resources[i]$tagPath == "[0]" && resources[i]$tagLabel == "[2]"){

      local at : BitString = resources[i]$value;
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$acknowledgedTransitions$to_offnormal = at$value[0];
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$acknowledgedTransitions$to_fault = at$value[1];
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$acknowledgedTransitions$to_normal = at$value[2];
    
    } else if(resources[i]$tagPath[0:6] == "[0][3]" && timeStampFlag == F){
      
      local t: any = parseBACnetTimeStamps(i, "[0][3]", 3);
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$eventTimeStamps = t;

      timeStampFlag = T;

    } else if(resources[i]$tagPath == "[0]" && resources[i]$tagLabel == "[4]"){

      local nt : Unsigned = resources[i]$value;
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$notifyType = BACnetAPDUTypes::BACnetNotifyType[nt$value];

    } else if(resources[i]$tagPath == "[0]" && resources[i]$tagLabel == "[5]"){

      local ee : BitString = resources[i]$value;
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$eventEnable$to_offnormal = ee$value[0];
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$eventEnable$to_fault = ee$value[1];
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$eventEnable$to_normal = ee$value[2];
    
    } else if(resources[i]$tagPath[0:6] == "[0][6]" && eventPrioritiesFlag == F){

      local u: Unsigned = resources[i]$value;
      local u2: Unsigned = resources[i+1]$value;
      local u3: Unsigned = resources[i+2]$value;
      getEventInformation_ACK_Info$listOfEventSummaries[|getEventInformation_ACK_Info$listOfEventSummaries| - 1]$eventPriorities = [u, u2, u3];

      eventPrioritiesFlag = T;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){
	
      local b : Boolean = resources[i]$value;
      getEventInformation_ACK_Info$moreEvents = b$value;

    }

  }

  local request_Info: Request_Info = popRequestInfo(invokeid, "getEventInformation");
  if(request_Info?$content){
    getEventInformation_ACK_Info$bacnetRequestInfo = request_Info$content;
  }

  event bacnet_GetEventInformation_ACK(c, invokeid, getEventInformation_ACK_Info);

}

############################################################################################
#  event bacnet_UnconfirmedCOVNotification_Request(c, unconfirmedCOVNotification_Request)  #
############################################################################################
export {

  type UnconfirmedCOVNotification_Request_Info : record {

    subscriberProcessIdentifier:	count;
    initiatingDeviceIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    monitoredObjectIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    timeRemaining:			count;
    listOfValues:			vector of BACnetAPDUTypes::BACnetPropertyValue;

  };

  global bacnet_UnconfirmedCOVNotification_Request: event(c: connection, info: UnconfirmedCOVNotification_Request_Info);

}

event bacnet_unconfirmed_cov_notification_request(c: connection) &priority = 5
{

  local unconfirmedCOVNotification_Request_Info: UnconfirmedCOVNotification_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local spi : Unsigned = resources[i]$value;
      unconfirmedCOVNotification_Request_Info$subscriberProcessIdentifier = spi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local idi : Object = resources[i]$value;
      unconfirmedCOVNotification_Request_Info$initiatingDeviceIdentifier = idi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){

      local moi : Object = resources[i]$value;
      unconfirmedCOVNotification_Request_Info$monitoredObjectIdentifier = moi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[3]"){

      local tr : Unsigned = resources[i]$value;
      unconfirmedCOVNotification_Request_Info$timeRemaining = tr$value;

    } else if(resources[i]$tagPath == "[4]" && resources[i]$tagLabel == "[0]"){

      local property : Property = resources[i]$value;
      unconfirmedCOVNotification_Request_Info$listOfValues[|unconfirmedCOVNotification_Request_Info$listOfValues|] = [$propertyIdentifier = property$value, $value = ""];

    } else if(resources[i]$tagPath == "[4]" && resources[i]$tagLabel == "[1]"){

      local pai : Unsigned = resources[i]$value;
      unconfirmedCOVNotification_Request_Info$listOfValues[|unconfirmedCOVNotification_Request_Info$listOfValues| - 1]$propertyArrayIndex = pai$value;

    } else if(resources[i]$tagPath[0:6] == "[4][2]"){

      unconfirmedCOVNotification_Request_Info$listOfValues[|unconfirmedCOVNotification_Request_Info$listOfValues| - 1]$value = parseAbstractSyntaxAndType(i, "[4][2]", unconfirmedCOVNotification_Request_Info$monitoredObjectIdentifier$id, unconfirmedCOVNotification_Request_Info$listOfValues[|unconfirmedCOVNotification_Request_Info$listOfValues| - 1]$propertyIdentifier);

      triggerInformationUnit(c, "UnconfirmedCOVNotification_Request", unconfirmedCOVNotification_Request_Info$monitoredObjectIdentifier, unconfirmedCOVNotification_Request_Info$listOfValues[|unconfirmedCOVNotification_Request_Info$listOfValues| - 1]$propertyIdentifier, unconfirmedCOVNotification_Request_Info$listOfValues[|unconfirmedCOVNotification_Request_Info$listOfValues| - 1]$value);

    } else if(resources[i]$tagPath == "[4]" && resources[i]$tagLabel == "[3]"){

      local p : Unsigned = resources[i]$value;
      unconfirmedCOVNotification_Request_Info$listOfValues[|unconfirmedCOVNotification_Request_Info$listOfValues| - 1]$priority = p$value;

    }

  }

  event bacnet_UnconfirmedCOVNotification_Request(c, unconfirmedCOVNotification_Request_Info);

}

#####################################################################################################
#  event bacnet_UnconfirmedEventNotification_Request(c, unconfirmedEventNotification_Request_Info)  #
#####################################################################################################
export {

  type UnconfirmedEventNotification_Request_Info : record {

    processIdentifier:			count;
    initiatingDeviceIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    eventObjectIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    timeStamp:				any;
    notificationClass:			count;
    priority:				count;
    eventType:				string;
    messageText:			string 						&optional;
    notifyType:				string;
    ackRequired:			bool						&optional;
    fromState:				string				 		&optional;
    toState:				string;
    eventValues:			any 						&optional;

  };

  global bacnet_UnconfirmedEventNotification_Request: event(c: connection, info: UnconfirmedEventNotification_Request_Info);

}

event bacnet_unconfirmed_event_notification_request(c: connection) &priority = 5
{

  local unconfirmedEventNotification_Request_Info: UnconfirmedEventNotification_Request_Info;

  local timeStampFlag: bool = F;
  local eventValuesFlag: bool = F;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local pi : Unsigned = resources[i]$value;
      unconfirmedEventNotification_Request_Info$processIdentifier = pi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local idi : Object = resources[i]$value;
      unconfirmedEventNotification_Request_Info$initiatingDeviceIdentifier = idi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){

      local eoi : Object = resources[i]$value;
      unconfirmedEventNotification_Request_Info$eventObjectIdentifier = eoi$value;

    } else if(resources[i]$tagPath[0:3] == "[3]" && timeStampFlag == F){

      local t: any = parseBACnetTimeStamps(i, "[3]", 1);
      unconfirmedEventNotification_Request_Info$timeStamp = t;

      timeStampFlag = T;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[4]"){

      local nc : Unsigned = resources[i]$value;
      unconfirmedEventNotification_Request_Info$notificationClass = nc$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[5]"){

      local p : Unsigned = resources[i]$value;
      unconfirmedEventNotification_Request_Info$priority = p$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[6]"){

      local et : Unsigned = resources[i]$value;
      unconfirmedEventNotification_Request_Info$eventType = BACnetAPDUTypes::BACnetEventType[et$value];

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[7]"){

      local mt : CharacterString = resources[i]$value;
      unconfirmedEventNotification_Request_Info$messageText = mt$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[8]"){

      local nt : Unsigned = resources[i]$value;
      unconfirmedEventNotification_Request_Info$notifyType = BACnetAPDUTypes::BACnetNotifyType[nt$value];

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[9]"){

      local ar : Boolean = resources[i]$value;
      unconfirmedEventNotification_Request_Info$ackRequired = ar$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[10]"){

      local fs : Unsigned = resources[i]$value;
      unconfirmedEventNotification_Request_Info$fromState = BACnetAPDUTypes::BACnetEventState[fs$value];

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[11]"){

      local ts : Unsigned = resources[i]$value;
      unconfirmedEventNotification_Request_Info$toState = BACnetAPDUTypes::BACnetEventState[ts$value];

    } else if(resources[i]$tagPath[0:4] == "[12]" && eventValuesFlag == F){

      local ev : any = parseBACnetNotificationParameters(i, "[12]");
      unconfirmedEventNotification_Request_Info$eventValues = ev;

      eventValuesFlag = T;

    }

  }

  triggerInformationUnit(c, "UnconfirmedEventNotification_Request", unconfirmedEventNotification_Request_Info$eventObjectIdentifier);
  
  event bacnet_UnconfirmedEventNotification_Request(c, unconfirmedEventNotification_Request_Info);

}

##################################################################################################
#   event bacnet_UnconfirmedPrivateTransfer_Request(c, unconfirmedPrivateTransfer_Request_Info)  #
##################################################################################################
export {

  type UnconfirmedPrivateTransfer_Request_Info : record {

    vendorID:		string;
    serviceNumber:	count;
    serviceParameters:	any		&optional;

  };

  global bacnet_UnconfirmedPrivateTransfer_Request: event(c: connection, unconfirmedPrivateTransfer_Request_Info: UnconfirmedPrivateTransfer_Request_Info);

}

event bacnet_unconfirmed_private_transfer_request(c: connection) &priority = 5
{

  local unconfirmedPrivateTransfer_Request_Info: UnconfirmedPrivateTransfer_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local vId : Unsigned = resources[i]$value;
      if(vId$value in BACnetAPDUTypes::BACnetVendorId){
        unconfirmedPrivateTransfer_Request_Info$vendorID = BACnetAPDUTypes::BACnetVendorId[vId$value];
      } else {
        unconfirmedPrivateTransfer_Request_Info$vendorID = fmt("Unknown Vendor (%s)", vId$value);
      }

   } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local sn : Unsigned = resources[i]$value;
      unconfirmedPrivateTransfer_Request_Info$serviceNumber = sn$value;

   } else if (resources[i]$tagPath[0:3] == "[2]"){

      unconfirmedPrivateTransfer_Request_Info$serviceParameters = parseAbstractSyntaxAndType(i, "[2]");

   }

  }

  triggerInformationUnit(c, "UnconfirmedPrivateTransfer_Request");
  
  event bacnet_UnconfirmedPrivateTransfer_Request(c, unconfirmedPrivateTransfer_Request_Info);

}

##############################################################################
#  event bacnet_TimeSynchronization_Request(c, timeSynchronization_Request)  #
##############################################################################
export {

  type TimeSynchronization_Request_Info : record {

    time_:		BACnetAPDUTypes::BACnetDateTime;

  };

  global bacnet_TimeSynchronization_Request: event(c: connection, info: TimeSynchronization_Request_Info);

}

event bacnet_time_synchronization_request(c: connection) &priority = 5
{

  local timeSynchronization_Request_Info: TimeSynchronization_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "Date"){

      timeSynchronization_Request_Info$time_ = parseBACnetDateTime(i, "");

    } 

  }

  triggerInformationUnit(c, "TimeSynchronization_Request");
  
  event bacnet_TimeSynchronization_Request(c, timeSynchronization_Request_Info);

}

#######################################################################################
#  event bacnet_AcknowledgeAlarm_Request(c, invokeid, acknowledgeAlarm_Request_Info)  #
#######################################################################################
export {

  type AcknowledgeAlarm_Request_Info : record {

    acknowledgingProcessIdentifier:	count;
    eventObjectIdentifier:		BACnetAPDUTypes::BACnetObjectIdentifier;
    eventStateAcknowledged:		string;
    timeStamp:				any;
    acknowledgmentSource:		string;
    timeOfAcknowledgement:		any;

  };

  global bacnet_AcknowledgeAlarm_Request: event(c: connection, invokeid: count, info: AcknowledgeAlarm_Request_Info);

}

event bacnet_acknowledge_alarm_request(c: connection, invokeid: count) &priority = 5
{

  local acknowledgeAlarm_Request_Info: AcknowledgeAlarm_Request_Info;

  local timeStampFlag: bool = F;
  local timeOfAcknowledgementFlag: bool = F;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local api : Unsigned = resources[i]$value;
      acknowledgeAlarm_Request_Info$acknowledgingProcessIdentifier = api$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local eoi : Object = resources[i]$value;
      acknowledgeAlarm_Request_Info$eventObjectIdentifier = eoi$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){

      local esa : Unsigned = resources[i]$value;
      acknowledgeAlarm_Request_Info$eventStateAcknowledged = BACnetAPDUTypes::BACnetEventState[esa$value];

    } else if(resources[i]$tagPath[0:3] == "[3]" && timeStampFlag == F){

      local t: any = parseBACnetTimeStamps(i, "[3]", 1);
      acknowledgeAlarm_Request_Info$timeStamp = t;

      timeStampFlag = T;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[4]"){

      local as : CharacterString = resources[i]$value;
      acknowledgeAlarm_Request_Info$acknowledgmentSource = as$value;

    } else if(resources[i]$tagPath[0:3] == "[5]" && timeOfAcknowledgementFlag == F){

      local toa: any = parseBACnetTimeStamps(i, "[5]", 1);
      acknowledgeAlarm_Request_Info$timeOfAcknowledgement = toa;

      timeOfAcknowledgementFlag = T;

    }

  }

  pushRequestInfo(invokeid, "acknowledgeAlarm", acknowledgeAlarm_Request_Info);
  
  triggerInformationUnit(c, "AcknowledgeAlarm_Request", acknowledgeAlarm_Request_Info$eventObjectIdentifier);
  
  event bacnet_AcknowledgeAlarm_Request(c, invokeid, acknowledgeAlarm_Request_Info);

}

##################################################################################
#   event bacnet_ReinitializeDevice_Request(c, reinitializeDevice_Request_Info)  #
##################################################################################
export {

  type ReinitializeDevice_Request_Info : record {

    reinitializedStateOfDevice:		string;
    password:				string		&optional;

  };

  global bacnet_ReinitializeDevice_Request: event(c: connection, invokeid: count, reinitializeDevice_Request_Info: ReinitializeDevice_Request_Info);

}

event bacnet_reinitialize_device_request(c: connection, invokeid: count) &priority = 5
{

  local reinitializeDevice_Request_Info: ReinitializeDevice_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local rsod : Enumerated = resources[i]$value;
      
      switch (rsod$value) {

        case 0:
          reinitializeDevice_Request_Info$reinitializedStateOfDevice = "coldstart";
          break;
        
        case 1:
          reinitializeDevice_Request_Info$reinitializedStateOfDevice = "warmstart";
          break;

        case 2:
          reinitializeDevice_Request_Info$reinitializedStateOfDevice = "startbackup";
          break;

        case 3:
          reinitializeDevice_Request_Info$reinitializedStateOfDevice = "endbackup";
          break;

        case 4:
          reinitializeDevice_Request_Info$reinitializedStateOfDevice = "startrestore";
          break;

        case 5:
          reinitializeDevice_Request_Info$reinitializedStateOfDevice = "endrestore";
          break;

        case 6:
          reinitializeDevice_Request_Info$reinitializedStateOfDevice = "abortrestore";
          break;

     }

   } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local p : CharacterString = resources[i]$value;
      reinitializeDevice_Request_Info$password = p$value;

   }

  }

  pushRequestInfo(invokeid, "reinitializeDevice", reinitializeDevice_Request_Info);
  
  triggerInformationUnit(c, "ReinitializeDevice_Request");
  
  event bacnet_ReinitializeDevice_Request(c, invokeid, reinitializeDevice_Request_Info);

}

###############################################################################
#  event bacnet_CreateObject_Request(c, invokeid, createObject_Request_Info)  #
###############################################################################
export {

  type CreateObject_Request_Info : record {

    objectSpecifier:			any;
    listOfInitialValues:		vector of BACnetAPDUTypes::BACnetPropertyValue		&optional;

  };

  global bacnet_CreateObject_Request: event(c: connection, invokeid: count, createObject_Request_Info: CreateObject_Request_Info);

}

event bacnet_create_object_request(c: connection, invokeid: count) &priority = 5
{

  local createObject_Request_Info: CreateObject_Request_Info;

  local object : BACnetAPDUTypes::BACnetObjectIdentifier;						#INFO: I need this for the "triggerInformationUnit" function

  local vectorFlag = F;
  for(i in resources){

    if(resources[i]$tagPath == "[0]" && resources[i]$tagLabel == "[0]"){

      local objectType : Enumerated = resources[i]$value;
      createObject_Request_Info$objectSpecifier = BACnetAPDUTypes::BACnetObjectType[objectType$value];

      object = [$id = createObject_Request_Info$objectSpecifier, $instance = 18446744073709551615];	#INFO: I need to pass something as default instance

    } else if(resources[i]$tagPath == "[0]" && resources[i]$tagLabel == "[1]"){

      local objectIdentifier : Object = resources[i]$value;
      createObject_Request_Info$objectSpecifier = objectIdentifier$value;

      object = createObject_Request_Info$objectSpecifier;

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[0]"){

      if(vectorFlag == F){
        createObject_Request_Info$listOfInitialValues = vector();
        vectorFlag = T;
      }

      local property : Property = resources[i]$value;
      createObject_Request_Info$listOfInitialValues[|createObject_Request_Info$listOfInitialValues|] = [$propertyIdentifier = property$value, $value = ""];

      triggerInformationUnit(c, "CreateObject_Request", object, createObject_Request_Info$listOfInitialValues[|createObject_Request_Info$listOfInitialValues| - 1]$propertyIdentifier);

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[1]"){

      local pai : Unsigned = resources[i]$value;
      createObject_Request_Info$listOfInitialValues[|createObject_Request_Info$listOfInitialValues| - 1]$propertyArrayIndex = pai$value;

    } else if(resources[i]$tagPath[0:6] == "[1][2]"){

      createObject_Request_Info$listOfInitialValues[|createObject_Request_Info$listOfInitialValues| - 1]$value = parseAbstractSyntaxAndType(i, "[1][2]", createObject_Request_Info$objectSpecifier, createObject_Request_Info$listOfInitialValues[|createObject_Request_Info$listOfInitialValues| - 1]$propertyIdentifier);

    } else if(resources[i]$tagPath == "[1]" && resources[i]$tagLabel == "[3]"){

      local p : Unsigned = resources[i]$value;
      createObject_Request_Info$listOfInitialValues[|createObject_Request_Info$listOfInitialValues| - 1]$priority = p$value;

    }

  }

  pushRequestInfo(invokeid, "createObject", createObject_Request_Info);

  event bacnet_CreateObject_Request(c, invokeid, createObject_Request_Info);

}

#######################################################################
#  event bacnet_CreateObject_ACK(c, invokeid, createObject_ACK_Info)  #
#######################################################################
export {

  type CreateObject_ACK_Info : record { 

    createObject_ACK:		BACnetAPDUTypes::BACnetObjectIdentifier;

    bacnetRequestInfo:		CreateObject_Request_Info			&optional;

  };

  global bacnet_CreateObject_ACK: event(c: connection, invokeid: count, createObject_ACK_Info: CreateObject_ACK_Info);

}

event bacnet_create_object_ack(c: connection, invokeid: count) &priority = 5
{

  local createObject_ACK_Info: CreateObject_ACK_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "BACnetObjectIdentifier"){

      local objectIdentifier : Object = resources[i]$value;
      createObject_ACK_Info$createObject_ACK = objectIdentifier$value;

    }

  }

  local request_Info: Request_Info = popRequestInfo(invokeid, "createObject");
  if(request_Info?$content){
    createObject_ACK_Info$bacnetRequestInfo = request_Info$content;
  }

  triggerInformationUnit(c, "CreateObject_ACK", createObject_ACK_Info$createObject_ACK); 
  
  event bacnet_CreateObject_ACK(c, invokeid, createObject_ACK_Info);

}

##########################################################
#   event bacnet_Who_Is_Request(c, who_Is_Request_Info)  #
##########################################################
export { 

  type Who_Is_Request_Info : record {

    deviceInstanceRangeLowLimit: 	count	&optional;
    deviceInstanceRangeHighLimit:	count	&optional;

  };

  global bacnet_Who_Is_Request: event(c: connection, who_Is_Request_Info: Who_Is_Request_Info);

}

event bacnet_who_is_request(c: connection) &priority = 5
{

  local who_Is_Request_Info: Who_Is_Request_Info;
  
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local lowLimit : Unsigned = resources[i]$value;
      who_Is_Request_Info$deviceInstanceRangeLowLimit = lowLimit$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local highLimit : Unsigned = resources[i]$value;
      who_Is_Request_Info$deviceInstanceRangeHighLimit = highLimit$value;

    }
  
  }

  if(who_Is_Request_Info?$deviceInstanceRangeLowLimit && who_Is_Request_Info?$deviceInstanceRangeHighLimit){
  
    local objectInstance : count = who_Is_Request_Info$deviceInstanceRangeHighLimit;
    while(objectInstance >= who_Is_Request_Info$deviceInstanceRangeLowLimit){
  
      local object : BACnetAPDUTypes::BACnetObjectIdentifier = [$id = "device", $instance = objectInstance];

      triggerInformationUnit(c, "Who_Is_Request", object);

      if(objectInstance == 0){
        break;
      }

      objectInstance = objectInstance - 1;

    }

  }

  event bacnet_Who_Is_Request(c, who_Is_Request_Info);

}

##################################################################################################################
#   event bacnet_I_Am_Request(c, iAmDeviceIdentifier, maxAPDULengthAccepted, segmentationSupported, vendorID)    #
##################################################################################################################
export { 

  type I_Am_Request_Info : record {

  iAmDeviceIdentifier:  	BACnetAPDUTypes::BACnetObjectIdentifier;
  maxAPDULengthAccepted:	count;
  segmentationSupported:	string; 
  vendorID:			string;

  };

  global bacnet_I_Am_Request: event(c: connection, i_Am_Request_Info: I_Am_Request_Info); 
}

event bacnet_i_am_request(c: connection) &priority = 5
{
  
  local i_Am_Request_Info: I_Am_Request_Info;
  
  local unsignedCounter: count = 0;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "BACnetObjectIdentifier"){

      local object : Object = resources[i]$value;
      i_Am_Request_Info$iAmDeviceIdentifier = object$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "Unsigned" && unsignedCounter == 0){

      local maxAPDU : Unsigned = resources[i]$value;
      i_Am_Request_Info$maxAPDULengthAccepted = maxAPDU$value;

      unsignedCounter += 1;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "ENUMERATED"){				#INFO: "BACnetSegmentation" has label ENUMERATED 

      local enumerated : Enumerated = resources[i]$value;
      i_Am_Request_Info$segmentationSupported = BACnetAPDUTypes::BACnetSegmentation[enumerated$value];

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "Unsigned" && unsignedCounter == 1){	#INFO: "Unsigned16" has label Unsigned

      local vId : Unsigned = resources[i]$value;
      if(vId$value in BACnetAPDUTypes::BACnetVendorId){
        i_Am_Request_Info$vendorID = BACnetAPDUTypes::BACnetVendorId[vId$value];
      } else {
        i_Am_Request_Info$vendorID = fmt("Unknown Vendor (%s)", vId$value);
      }

    }

  }

  triggerInformationUnit(c, "I_Am_Request", i_Am_Request_Info$iAmDeviceIdentifier, "vendor-identifier", i_Am_Request_Info$vendorID);
  
  event bacnet_I_Am_Request(c, i_Am_Request_Info);

}

############################################################
#   event bacnet_Who_Has_Request(c, who_Has_Request_Info)  #
############################################################
export {

  type Who_Has_Request_Info : record {

    deviceInstanceRangeLowLimit:	count						&optional;
    deviceInstanceRangeHighLimit:	count						&optional;
    objectIdentifier:			BACnetAPDUTypes::BACnetObjectIdentifier		&optional;
    objectName:				string						&optional;

  };

  global bacnet_Who_Has_Request: event(c: connection, who_Has_Request_Info: Who_Has_Request_Info);

}

event bacnet_who_has_request(c: connection) &priority = 5
{
  
  local who_Has_Request_Info: Who_Has_Request_Info;

  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[0]"){

      local lowLimit : Unsigned = resources[i]$value;
      who_Has_Request_Info$deviceInstanceRangeLowLimit = lowLimit$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[1]"){

      local highLimit : Unsigned = resources[i]$value;
      who_Has_Request_Info$deviceInstanceRangeHighLimit = highLimit$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[2]"){

      local objectIdentifier : Object = resources[i]$value;
      who_Has_Request_Info$objectIdentifier = objectIdentifier$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "[3]"){

      local name : CharacterString = resources[i]$value;
      who_Has_Request_Info$objectName = name$value;

    }

  }

  if(who_Has_Request_Info?$deviceInstanceRangeLowLimit && who_Has_Request_Info?$deviceInstanceRangeHighLimit){
  
    local objectInstance : count = who_Has_Request_Info$deviceInstanceRangeHighLimit;
    while(objectInstance >= who_Has_Request_Info$deviceInstanceRangeLowLimit){
  
      local object : BACnetAPDUTypes::BACnetObjectIdentifier = [$id = "device", $instance = objectInstance];

      triggerInformationUnit(c, "Who_Has_Request", object);
  
      if(objectInstance == 0){
        break;
      }    

      objectInstance = objectInstance - 1;

    }

  }

  event bacnet_Who_Has_Request(c, who_Has_Request_Info);

}

##########################################################
#   event bacnet_I_Have_Request(c, i_Have_Request_Info)  #
##########################################################
export {

  type I_Have_Request_Info : record {

    deviceIdentifier:			BACnetAPDUTypes::BACnetObjectIdentifier;
    objectIdentifier:			BACnetAPDUTypes::BACnetObjectIdentifier;
    objectName:				string;

  };

  global bacnet_I_Have_Request: event(c: connection, i_Have_Request_Info: I_Have_Request_Info);

}

event bacnet_i_have_request(c: connection) &priority = 5
{
  
  local i_Have_Request_Info: I_Have_Request_Info;

  local objectCounter: count = 0;
  for(i in resources){

    if(resources[i]$tagPath == "" && resources[i]$tagLabel == "BACnetObjectIdentifier" && objectCounter == 0){

      local device : Object = resources[i]$value;
      i_Have_Request_Info$deviceIdentifier = device$value;

      objectCounter += 1;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "BACnetObjectIdentifier" && objectCounter == 1){

      local object : Object = resources[i]$value;
      i_Have_Request_Info$objectIdentifier = object$value;

    } else if(resources[i]$tagPath == "" && resources[i]$tagLabel == "CharacterString"){

      local name : CharacterString = resources[i]$value;
      i_Have_Request_Info$objectName = name$value;

    }

  }

  triggerInformationUnit(c, "I_Have_Request", i_Have_Request_Info$deviceIdentifier);
  triggerInformationUnit(c, "I_Have_Request", i_Have_Request_Info$objectIdentifier, "object-name", i_Have_Request_Info$objectName);
  
  event bacnet_I_Have_Request(c, i_Have_Request_Info);

}

################################################################################################

function extractInformationUnit(service: string, content: any, isrequest: bool &default = T): InformationUnit
{
  
  switch (service) {
 
    case "readProperty":
      
      if(isrequest){
        
 	local readProperty_Request_Info: ReadProperty_Request_Info = content;
        return [$message = "ReadProperty_Request", $object = readProperty_Request_Info$objectIdentifier, $property = readProperty_Request_Info$propertyIdentifier];

      } 
      
      local readProperty_ACK_Info: ReadProperty_ACK_Info = content;
      return [$message = "ReadProperty_ACK", $object = readProperty_ACK_Info$objectIdentifier, $property = readProperty_ACK_Info$propertyIdentifier, $value = readProperty_ACK_Info$propertyValue];

    case "readPropertyMultiple":
      
      if(isrequest){
        
 	local readPropertyMultiple_Request_Info: ReadPropertyMultiple_Request_Info = content;
        return [$message = "readPropertyMultiple_Request"];						# FIXME: This should carry multiple information units

      } 
      
      local readPropertyMultiple_ACK_Info: ReadPropertyMultiple_ACK_Info = content;
      return [$message = "readPropertyMultiple_ACK"];							# FIXME: This should carry multiple information units

    case "readRange":
      
      if(isrequest){
        
 	local readRange_Request_Info: ReadRange_Request_Info = content;
        return [$message = "ReadRange_Request", $object = readRange_Request_Info$objectIdentifier, $property = readRange_Request_Info$propertyIdentifier];

      } 
      
      local readRange_ACK_Info: ReadRange_ACK_Info = content;
      return [$message = "ReadRange_ACK", $object = readRange_ACK_Info$objectIdentifier, $property = readRange_ACK_Info$propertyIdentifier];

    case "atomicReadFile":
      
      if(isrequest){
        
 	local atomicReadFile_Request_Info: AtomicReadFile_Request_Info = content;
        return [$message = "AtomicReadFile_Request", $object = atomicReadFile_Request_Info$fileIdentifier];

      } 
      
      local atomicReadFile_ACK_Info: AtomicReadFile_ACK_Info = content;
      return [$message = "AtomicReadFile_ACK"];

    case "subscribeCOV":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local subscribeCOV_Request_Info: SubscribeCOV_Request_Info = content;
        return [$message = "SubscribeCOV_Request", $object = subscribeCOV_Request_Info$monitoredObjectIdentifier];

      #}

    case "subscribeCOVProperty":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local subscribeCOVProperty_Request_Info: SubscribeCOVProperty_Request_Info = content;
        return [$message = "SubscribeCOVProperty_Request", $object = subscribeCOVProperty_Request_Info$monitoredObjectIdentifier, $property = subscribeCOVProperty_Request_Info$monitoredPropertyIdentifier$propertyIdentifier];

      #}

    case "confirmedCOVNotification":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local confirmedCOVNotification_Request_Info: ConfirmedCOVNotification_Request_Info = content;
        return [$message = "ConfirmedCOVNotification_Request", $object = confirmedCOVNotification_Request_Info$monitoredObjectIdentifier];

      #}

    case "confirmedEventNotification":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local confirmedEventNotification_Request_Info: ConfirmedEventNotification_Request_Info = content;
        return [$message = "ConfirmedEventNotification_Request", $object = confirmedEventNotification_Request_Info$eventObjectIdentifier];

      #}

    case "confirmedPrivateTransfer":
      
      if(isrequest){
        
 	local confirmedPrivateTransfer_Request_Info: ConfirmedPrivateTransfer_Request_Info = content;
        return [$message = "ConfirmedPrivateTransfer_Request"];

      }

      local confirmedPrivateTransfer_ACK_Info: ConfirmedPrivateTransfer_ACK_Info = content;
      return [$message = "ConfirmedPrivateTransfer_ACK"];

    case "writeProperty":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local writeProperty_Request_Info: WriteProperty_Request_Info = content;
        return [$message = "WriteProperty_Request", $object = writeProperty_Request_Info$objectIdentifier, $property = writeProperty_Request_Info$propertyIdentifier, $value = writeProperty_Request_Info$propertyValue];

      #}

    case "writePropertyMultiple":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local writePropertyMultiple_Request_Info: WritePropertyMultiple_Request_Info = content;
        return [$message = "WritePropertyMultiple_Request"];						# FIXME: This should carry multiple information units

      #}

    case "atomicWriteFile":
      
      if(isrequest){
        
 	local atomicWriteFile_Request_Info: AtomicWriteFile_Request_Info = content;
        return [$message = "AtomicWriteFile_Request", $object = atomicWriteFile_Request_Info$fileIdentifier];

      }

      local atomicWriteFile_ACK_Info: AtomicWriteFile_ACK_Info = content;
      return [$message = "AtomicWriteFile_ACK"];

    case "getEventInformation":
      
      if(isrequest){
        
 	local getEventInformation_Request_Info: GetEventInformation_Request_Info = content;
        if(getEventInformation_Request_Info?$lastReceivedObjectIdentifier){
          return [$message = "GetEventInformation_Request", $object = getEventInformation_Request_Info$lastReceivedObjectIdentifier];
        } else{
          return [$message = "GetEventInformation_Request"];
        }     

      }

      local getEventInformation_ACK_Info: GetEventInformation_ACK_Info = content;
      return [$message = "GetEventInformation_ACK"];							# FIXME: This should carry multiple information units

    case "unconfirmedCOVNotification":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local unconfirmedCOVNotification_Request_Info: UnconfirmedCOVNotification_Request_Info = content;
        return [$message = "UnconfirmedCOVNotification_Request", $object = unconfirmedCOVNotification_Request_Info$monitoredObjectIdentifier];						

      #}

    case "unconfirmedEventNotification":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local unconfirmedEventNotification_Request_Info: UnconfirmedEventNotification_Request_Info = content;
        return [$message = "UnconfirmedEventNotification_Request", $object = unconfirmedEventNotification_Request_Info$eventObjectIdentifier];						

      #}

    case "unconfirmedPrivateTransfer":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local unconfirmedPrivateTransfer_Request_Info: UnconfirmedPrivateTransfer_Request_Info = content;
        return [$message = "UnconfirmedPrivateTransfer_Request"];						

      #}

    case "timeSynchronization":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local timeSynchronization_Request_Info: TimeSynchronization_Request_Info = content;
        return [$message = "TimeSynchronization_Request"];						

      #}

    case "acknowledgeAlarm":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local acknowledgeAlarm_Request_Info: AcknowledgeAlarm_Request_Info = content;
        return [$message = "AcknowledgeAlarm_Request", $object = acknowledgeAlarm_Request_Info$eventObjectIdentifier];						

      #}

    case "reinitializeDevice":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local reinitializeDevice_Request_Info: ReinitializeDevice_Request_Info = content;
        return [$message = "ReinitializeDevice_Request"];						

      #}

    case "createObject":
      
      if(isrequest){
        
 	local createObject_Request_Info: CreateObject_Request_Info = content;
        return [$message = "CreateObject_Request"];						# FIXME: This should carry multiple information units

      }

      local createObject_ACK_Info: CreateObject_ACK_Info = content;
      return [$message = "CreateObject_ACK", $object = createObject_ACK_Info$createObject_ACK];

    case "who-Is":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local who_Is_Request_Info: Who_Is_Request_Info = content;
        return [$message = "Who_Is_Request"];							# FIXME: This should carry multiple information units				

      #}

    case "i-Am":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local i_Am_Request_Info: I_Am_Request_Info = content;
        return [$message = "I_Am_Request", $object = i_Am_Request_Info$iAmDeviceIdentifier];				

      #}

    case "who-Has":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local who_Has_Request_Info: Who_Has_Request_Info = content;
        return [$message = "Who_Has_Request"];							# FIXME: This should carry multiple information units				

      #}

    case "i-Have":
      
      #if(isrequest){			# INFO: Always TRUE
        
 	local i_Have_Request_Info: I_Have_Request_Info = content;
        return [$message = "I_Have_Request", $object = i_Have_Request_Info$deviceIdentifier];				

      #}

  }

}

