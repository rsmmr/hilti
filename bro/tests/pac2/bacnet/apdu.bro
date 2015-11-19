module BACnetTest;

@load bacnet_apdu.bro

# Test Log

export {

  redef enum Log::ID += { DEVICES };

  type Info_DEVICES: record {

    device:	string &log; 
    modelName:	string &log &optional;
    objectName:	string &log &optional;
  
  };

}

event bro_init()
{
  Log::create_stream(DEVICES, [$columns=Info_DEVICES]);
}

# Test ReadProperty_Request
event bacnet_ReadProperty_Request(c: connection, invokeid: count, info: BACnetAPDU::ReadProperty_Request_Info)
{ 
  print fmt("ReadProperty_Request [%s] -> Object: %s (%s), Property: \"%s\"", invokeid, info$objectIdentifier$instance, info$objectIdentifier$id, info$propertyIdentifier);
}

# Test ReadProperty_ACK
event bacnet_ReadProperty_ACK(c: connection, invokeid: count, info: BACnetAPDU::ReadProperty_ACK_Info)
{ 
  print fmt("ReadProperty_ACK [%s] -> Object: %s (%s), Property: \"%s\", Value: \"%s\"", invokeid, info$objectIdentifier$instance, info$objectIdentifier$id, info$propertyIdentifier, info$propertyValue);

  # Logging on DEVICES
  if(info$objectIdentifier$id == "device" && info$propertyIdentifier == "model-name") {
    Log::write(BACnetTest::DEVICES, [$device = fmt("%s", info$objectIdentifier$instance), $modelName = fmt("%s", info$propertyValue)]);
  } else if(info$objectIdentifier$id == "device" && info$propertyIdentifier == "object-name") {
    Log::write(BACnetTest::DEVICES, [$device = fmt("%s", info$objectIdentifier$instance), $objectName = fmt("%s", info$propertyValue)]);
  }

}

# Test ReadPropertyMultiple_Request
event bacnet_ReadPropertyMultiple_Request(c: connection, invokeid: count, info: BACnetAPDU::ReadPropertyMultiple_Request_Info)
{ 
  
  print fmt("ReadPropertyMultiple_Request [%s]", invokeid);
  
  for(i in info$listOfReadAccessSpecs){
    
    local accessSpecification: BACnetAPDUTypes::ReadAccessSpecification = info$listOfReadAccessSpecs[i];

    local res: string = fmt("  Object: %s (%s) -> [", accessSpecification$objectIdentifier$instance, accessSpecification$objectIdentifier$id);
    local listOfPropertyReferences = accessSpecification$listOfPropertyReferences;
    
    for(j in listOfPropertyReferences){

      local propertyReference: BACnetAPDUTypes::BACnetPropertyReference = listOfPropertyReferences[j];
      
      res += fmt("\"%s\", ", propertyReference$propertyIdentifier);
    
    }

    res = res[0:|res|-2] + "]";
    print res;

  }

}

# Test ReadPropertyMultiple_Ack
event bacnet_ReadPropertyMultiple_ACK(c: connection, invokeid: count, info: BACnetAPDU::ReadPropertyMultiple_ACK_Info)
{ 
  
  print fmt("ReadPropertyMultiple_ACK [%s]", invokeid);
  
  for(i in info$listOfReadAccessResults){
    
    local result: BACnetAPDUTypes::ReadAccessResult = info$listOfReadAccessResults[i];

    local res: string = fmt("  Object: %s (%s) -> [", result$objectIdentifier$instance, result$objectIdentifier$id);

    local objectName_log: string = "";
    local modelName_log: string = "";

    for(j in result$listOfResults){
      
      if(result$listOfResults[j]?$propertyValue){
        res += fmt("\"%s\" = \"%s\", ", result$listOfResults[j]$propertyIdentifier, result$listOfResults[j]$propertyValue);
      } else if(result$listOfResults[j]?$propertyAccessError){
        res += fmt("\"%s\" = \"%s\", ", result$listOfResults[j]$propertyIdentifier, result$listOfResults[j]$propertyAccessError);
      }
    
      if(result$listOfResults[j]$propertyIdentifier == "model-name") {
        modelName_log = result$listOfResults[j]$propertyValue;
      } else if(result$listOfResults[j]$propertyIdentifier == "object-name") {
        objectName_log = result$listOfResults[j]$propertyValue;
      }

    }

    res = res[0:|res|-2] + "]";
    print res;

    #Logging on DEVICES
    if(result$objectIdentifier$id == "device" && (modelName_log != "" || objectName_log != "")){
      Log::write(BACnetTest::DEVICES, [$device = fmt("%s", result$objectIdentifier$instance), $modelName = fmt("%s", modelName_log), $objectName = fmt("%s", objectName_log)]);
      }

  }

}

# Test ReadRange_Request
event bacnet_ReadRange_Request(c: connection, invokeid: count, info: BACnetAPDU::ReadRange_Request_Info)
{
  print fmt("ReadRange_Request [%s] -> Object: %s (%s), Property: \"%s\"", invokeid, info$objectIdentifier$instance, info$objectIdentifier$id, info$propertyIdentifier);
  if(info?$range){
    print fmt("  Range: %s", info$range);
  }
}

# Test ReadRange_ACK
event bacnet_ReadRange_ACK(c: connection, invokeid: count, info: BACnetAPDU::ReadRange_ACK_Info)
{
  print fmt("ReadRange_ACK [%s] -> Object: %s (%s), Property: \"%s\"", invokeid, info$objectIdentifier$instance, info$objectIdentifier$id, info$propertyIdentifier);
  print fmt("  Result Flags: %s", info$resultFlags);
}

# Test AtomicReadFile_Request
event bacnet_AtomicReadFile_Request(c: connection, invokeid: count, info: BACnetAPDU::AtomicReadFile_Request_Info)
{
  print fmt("AtomicReadFile_Request [%s] -> Object: %s, Access: %s", invokeid, info$fileIdentifier$instance, info$accessMethod);
}

# Test AtomicReadFile_ACK
event bacnet_AtomicReadFile_ACK(c: connection, invokeid: count, info: BACnetAPDU::AtomicReadFile_ACK_Info)
{
  print fmt("AtomicReadFile_ACK [%s] -> End Of File: %s, Access: %s", invokeid, info$endOfFile, info$accessMethod);
}

# Test SubscribeCOV_Request
event bacnet_SubscribeCOV_Request(c: connection, invokeid: count, info: BACnetAPDU::SubscribeCOV_Request_Info)
{
  print fmt("SubscribeCOV_Request [%s] -> Subscriber: %s, Monitored Object: %s (%s)", invokeid, info$subscriberProcessIdentifier, info$monitoredObjectIdentifier$instance, info$monitoredObjectIdentifier$id);
}

# Test SubscribeCOVProperty_Request
event bacnet_SubscribeCOVProperty_Request(c: connection, invokeid: count, info: BACnetAPDU::SubscribeCOVProperty_Request_Info)
{
  print fmt("SubscribeCOVProperty_Request [%s] -> Subscriber: %s, Monitored Object: %s (%s), Monitored Property: %s", invokeid, info$subscriberProcessIdentifier, info$monitoredObjectIdentifier$instance, info$monitoredObjectIdentifier$id, info$monitoredPropertyIdentifier);
}

# Test ConfirmedCOVNotification_Request
event bacnet_ConfirmedCOVNotification_Request(c: connection, invokeid: count, info: BACnetAPDU::ConfirmedCOVNotification_Request_Info)
{
  
  print fmt("ConfirmedCOVNotification_Request [%s] -> Subscriber: %s, Initiating Device: %s (%s), Monitored Object: %s (%s), Time Remaining: %s", invokeid, info$subscriberProcessIdentifier, info$initiatingDeviceIdentifier$instance, info$initiatingDeviceIdentifier$id, info$monitoredObjectIdentifier$instance, info$monitoredObjectIdentifier$id, info$timeRemaining);
  
  for(i in info$listOfValues){
    print fmt("  Property: %s = \"%s\"", info$listOfValues[i]$propertyIdentifier, info$listOfValues[i]$value);
  }

}

# Test ConfirmedEventNotification_Request
event bacnet_ConfirmedEventNotification_Request(c: connection, invokeid: count, info: BACnetAPDU::ConfirmedEventNotification_Request_Info)
{
  print fmt("ConfirmedEventNotification_Request [%s] -> Process: %s, Initiating Device %s (%s), Event Object %s (%s)", invokeid, info$processIdentifier, info$initiatingDeviceIdentifier$instance, info$initiatingDeviceIdentifier$id, info$eventObjectIdentifier$instance, info$eventObjectIdentifier$id);
  print fmt("  Timestamp: %s", info$timeStamp);
  print fmt("  Notification Class: %s, Priority: %s, Event Type: \"%s\", Notify Type: \"%s\", To State: \"%s\"", info$notificationClass, info$priority, info$eventType, info$notifyType, info$toState);
}

# Test ConfirmedPrivateTransfer_Request
event bacnet_ConfirmedPrivateTransfer_Request(c: connection, invokeid: count, info: BACnetAPDU::ConfirmedPrivateTransfer_Request_Info)
{
  print fmt("ConfirmedPrivateTransfer_Request [%s] -> Vendor: %s, Service Number: %s", invokeid, info$vendorID, info$serviceNumber);
}

# Test ConfirmedPrivateTransfer_ACK
event bacnet_ConfirmedPrivateTransfer_ACK(c: connection, invokeid: count, info: BACnetAPDU::ConfirmedPrivateTransfer_ACK_Info)
{
  print fmt("ConfirmedPrivateTransfer_ACK [%s] -> Vendor: %s, Service Number: %s", invokeid, info$vendorID, info$serviceNumber);
}

# Test WriteProperty_Request
event bacnet_WriteProperty_Request(c: connection, invokeid: count, info: BACnetAPDU::WriteProperty_Request_Info)
{ 
  print fmt("WriteProperty_Request [%s] -> Object: %s (%s), Property: \"%s\" = \"%s\"", invokeid, info$objectIdentifier$instance, info$objectIdentifier$id, info$propertyIdentifier, info$propertyValue);
}

# Test WritePropertyMultiple_Request
event bacnet_WritePropertyMultiple_Request(c: connection, invokeid: count, info: BACnetAPDU::WritePropertyMultiple_Request_Info)
{ 

  print fmt("WritePropertyMultiple_ACK [%s]", invokeid);
  
  for(i in info$listOfwriteAccessSpecifications){
    
    local result: BACnetAPDUTypes::WriteAccessSpecification = info$listOfwriteAccessSpecifications[i];

    local res: string = fmt("  Object: %s (%s) -> [", result$objectIdentifier$instance, result$objectIdentifier$id);

    for(j in result$listOfProperties){
      
      res += fmt("\"%s\" = \"%s\", ", result$listOfProperties[j]$propertyIdentifier, result$listOfProperties[j]$value);
    
    }

    res = res[0:|res|-2] + "]";
    print res;

  }

}

# Test AtomicWriteFile_Request
event bacnet_AtomicWriteFile_Request(c: connection, invokeid: count, info: BACnetAPDU::AtomicWriteFile_Request_Info)
{
  print fmt("AtomicWriteFile_Request [%s] -> Object: %s, Access: %s", invokeid, info$fileIdentifier$instance, info$accessMethod);
}

# Test AtomicWriteFile_ACK
event bacnet_AtomicWriteFile_ACK(c: connection, invokeid: count, info: BACnetAPDU::AtomicWriteFile_ACK_Info)
{
  if(info?$fileStartPosition){
    print fmt("AtomicWriteFile_ACK [%s] -> Start Position: %s", invokeid, info$fileStartPosition);
  } else if(info?$fileStartRecord){
    print fmt("AtomicWriteFile_ACK [%s] -> Start Record: %s", invokeid, info$fileStartRecord);
  }
}

# Test GetEventInformation_Request
event bacnet_GetEventInformation_Request(c: connection, invokeid: count, info: BACnetAPDU::GetEventInformation_Request_Info)
{
  if(info?$lastReceivedObjectIdentifier){
    print fmt("GetEventInformation_Request [%s] -> Object: %s (%s)", invokeid, info$lastReceivedObjectIdentifier$instance, info$lastReceivedObjectIdentifier$id);
  } else {
    print fmt("GetEventInformation_Request [%s]", invokeid);
  }
}

# Test GetEventInformation_ACK
event bacnet_GetEventInformation_ACK(c: connection, invokeid: count, info: BACnetAPDU::GetEventInformation_ACK_Info)
{

  print fmt("GetEventInformation_ACK [%s]", invokeid);

  for(i in info$listOfEventSummaries){

    print fmt("  Object: %s (%s)", info$listOfEventSummaries[i]$objectIdentifier$instance, info$listOfEventSummaries[i]$objectIdentifier$id);
    print fmt("    Event State: %s, Acknowledged Transitions: %s", info$listOfEventSummaries[i]$eventState, info$listOfEventSummaries[i]$acknowledgedTransitions);
    print fmt("    Event Timestamps: %s", info$listOfEventSummaries[i]$eventTimeStamps);
    print fmt("    Notify Type: %s, Event Enable: %s, Event Priorities: %s", info$listOfEventSummaries[i]$notifyType, info$listOfEventSummaries[i]$eventEnable, info$listOfEventSummaries[i]$eventPriorities);

  }

  print fmt("  More Events: %s", info$moreEvents);

}

# Test UnconfirmedCOVNotification_Request
event bacnet_UnconfirmedCOVNotification_Request(c: connection, info: BACnetAPDU::UnconfirmedCOVNotification_Request_Info)
{
  
  print fmt("UnconfirmedCOVNotification_Request -> Subscriber: %s, Initiating Device: %s (%s), Monitored Object: %s (%s), Time Remaining: %s", info$subscriberProcessIdentifier, info$initiatingDeviceIdentifier$instance, info$initiatingDeviceIdentifier$id, info$monitoredObjectIdentifier$instance, info$monitoredObjectIdentifier$id, info$timeRemaining);
  
  for(i in info$listOfValues){
    print fmt("  Property: %s = \"%s\"", info$listOfValues[i]$propertyIdentifier, info$listOfValues[i]$value);
  }

}

# Test UnconfirmedEventNotification_Request
event bacnet_UnconfirmedEventNotification_Request(c: connection, info: BACnetAPDU::UnconfirmedEventNotification_Request_Info)
{
  print fmt("UnconfirmedEventNotification_Request -> Process: %s, Initiating Device %s (%s), Event Object %s (%s)", info$processIdentifier, info$initiatingDeviceIdentifier$instance, info$initiatingDeviceIdentifier$id, info$eventObjectIdentifier$instance, info$eventObjectIdentifier$id);
  print fmt("  Timestamp: %s", info$timeStamp);
  print fmt("  Notification Class: %s, Priority: %s, Event Type: \"%s\", Notify Type: \"%s\", To State: \"%s\"", info$notificationClass, info$priority, info$eventType, info$notifyType, info$toState);
}

# Test UnconfirmedPrivateTransfer_Request
event bacnet_UnconfirmedPrivateTransfer_Request(c: connection, info: BACnetAPDU::UnconfirmedPrivateTransfer_Request_Info)
{
  print fmt("UnconfirmedPrivateTransfer_Request -> Vendor: %s, Service Number: %s", info$vendorID, info$serviceNumber);
}

# Test TimeSynchronization_Request
event bacnet_TimeSynchronization_Request(c: connection, info: BACnetAPDU::TimeSynchronization_Request_Info)
{
  print fmt("TimeSynchronization_Request -> Date: %s", info$time_);
}

# Test AcknowledgeAlarm_Request
event bacnet_AcknowledgeAlarm_Request(c: connection, invokeid: count, info: BACnetAPDU::AcknowledgeAlarm_Request_Info)
{
  print fmt("AcknowledgeAlarm_Request [%s] -> Process: %s, Event Object %s (%s)", invokeid, info$acknowledgingProcessIdentifier, info$eventObjectIdentifier$instance, info$eventObjectIdentifier$id);
  print fmt("  Timestamp: %s", info$timeStamp);
  print fmt("  Source: \"%s\"", info$acknowledgmentSource);
  print fmt("  Time of Acknowledgment: %s", info$timeOfAcknowledgement);
}

# Test ReinitializeDevice_Request
event bacnet_ReinitializeDevice_Request(c: connection, invokeid: count, info: BACnetAPDU::ReinitializeDevice_Request_Info)
{
  
  print fmt("ReinitializeDevice_Request [%s] -> State: %s", invokeid, info$reinitializedStateOfDevice);
  
  if(info?$password){
    print fmt("  Password: %s", info$password);
  }

}

# Test CreateObject_Request
event bacnet_CreateObject_Request(c: connection, invokeid: count, info: BACnetAPDU::CreateObject_Request_Info)
{
  
  print fmt("CreateObject_Request [%s] -> Object: %s", invokeid, info$objectSpecifier);
  
  for(i in info$listOfInitialValues){
    print fmt("  Property: %s = \"%s\"", info$listOfInitialValues[i]$propertyIdentifier, info$listOfInitialValues[i]$value);
  }

}

# Test CreateObject_ACK
event bacnet_CreateObject_ACK(c: connection, invokeid: count, info: BACnetAPDU::CreateObject_ACK_Info)
{
  print fmt("CreateObject_ACK [%s] -> Object: %s", invokeid, info$createObject_ACK);
}

# Test Who_Is_Request
event bacnet_Who_Is_Request(c: connection, info: BACnetAPDU::Who_Is_Request_Info)
{
  if(info?$deviceInstanceRangeLowLimit && info?$deviceInstanceRangeHighLimit){
    print fmt("Who_Is_Request -> Low Limit: %s, High Limit: %s", info$deviceInstanceRangeLowLimit, info$deviceInstanceRangeHighLimit);
  } else {
    print fmt("Who_Is_Request");
  }
}

# Test I_Am_Request
event bacnet_I_Am_Request(c: connection, info: BACnetAPDU::I_Am_Request_Info)
{
  print fmt("I_Am_Request -> Object: %s (%s), Vendor: %s", info$iAmDeviceIdentifier$instance, info$iAmDeviceIdentifier$id, info$vendorID);
}

# Test Who_Has_Request
event bacnet_Who_Has_Request(c: connection, info: BACnetAPDU::Who_Has_Request_Info)
{
  if(info?$deviceInstanceRangeLowLimit && info?$deviceInstanceRangeHighLimit){
    if(info?$objectIdentifier){
      print fmt("Who_Has_Request -> Low Limit: %s, High Limit: %s, Object: %s (%s)", info$deviceInstanceRangeLowLimit, info$deviceInstanceRangeHighLimit, info$objectIdentifier$instance, info$objectIdentifier$id);
    } else if(info?$objectName){
      print fmt("Who_Has_Request -> Low Limit: %s, High Limit: %s, Name: %s ", info$deviceInstanceRangeLowLimit, info$deviceInstanceRangeHighLimit, info$objectName);
    }
  } else {
    if(info?$objectIdentifier){
      print fmt("Who_Has_Request -> Object: %s (%s)", info$objectIdentifier$instance, info$objectIdentifier$id);
    } else if(info?$objectName){
      print fmt("Who_Has_Request -> Name: %s ", info$objectName);
    }
  }
}

# Test I_Have_Request
event bacnet_I_Have_Request(c: connection, info: BACnetAPDU::I_Have_Request_Info)
{
  print fmt("I_Have_Request -> Device: %s (%s), Object: %s (%s) \"%s\"", info$deviceIdentifier$instance, info$deviceIdentifier$id, info$objectIdentifier$instance, info$objectIdentifier$id, info$objectName);
}

################################################################################################

# Test Error
event bacnet_Error (c: connection, invokeid: count, error_Info: BACnetAPDU::Error_Info)
{
  print fmt("Error [%s] -> Service: \"%s\", Class: \"%s\", Code: \"%s\"", invokeid, error_Info$service, error_Info$errorClass, error_Info$errorCode);
  print fmt("  Request Information: %s", error_Info$bacnetRequestInfo);
}

# Test Simple ACK
event bacnet_SimpleACK (c: connection, invokeid: count, simpleACK_Info: BACnetAPDU::SimpleACK_Info)
{
  print fmt("Simple ACK [%s] -> Service: \"%s\"", invokeid, simpleACK_Info$service);
  print fmt("  Request Information: %s", simpleACK_Info$bacnetRequestInfo);
}

################################################################################################

# Test NPDU
event bacnet_npdu_parameters(c: connection, src: string, dst: string)
{
  #print fmt("NPDU -> Src: %s, Dst: %s", src, dst);
}


