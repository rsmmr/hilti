module BACnetAPDUTypes;

export {

  type Date : record {

    year: 	count;
    month: 	count;
    day: 	count;
    weekday: 	count;

  };

  type Time : record {

    hour: 		count;
    minute: 		count;
    second: 		count;
    centisecond: 	count;

  };

  type BACnetDateTime : record {

    date:	Date;
    time_:	Time;

  };

  type Error : record {

    error_class:	string;
    error_code:		string;

  };

  type BACnetObjectIdentifier : record {
  
    id: 	string;
    instance: 	count;

  };

  type BACnetPropertyReference : record {

    propertyIdentifier: string;
    propertyArrayIndex: count 	&optional;

  };

  type BACnetPropertyValue : record {

    propertyIdentifier: 	string;
    propertyArrayIndex: 	count		&optional;
    value:			any;
    priority:			count		&optional;

  };

  type ReadAccessSpecification : record {

    objectIdentifier: 		BACnetObjectIdentifier;
    listOfPropertyReferences: 	vector of BACnetPropertyReference;

  };

  type ReadAccessResult : record {

    objectIdentifier: 	BACnetObjectIdentifier;
    listOfResults: 	vector of record {
                                     		propertyIdentifier: 	string;
                                     		propertyArrayIndex: 	count 	&optional;
                                    		propertyValue: 		any 	&optional;
                                     		propertyAccessError: 	Error 	&optional;
				    	 };
  };

  type WriteAccessSpecification : record {

    objectIdentifier:	BACnetObjectIdentifier;
    listOfProperties:	vector of BACnetPropertyValue;

  };

  type BACnetEventTransitionBits : record {

    to_offnormal:	bool;
    to_fault:		bool;
    to_normal:		bool;

  };

  type BACnetResultFlags : record {

    first_item:		bool;
    last_item:		bool;
    more_item:		bool;

  };

  type BACnetStatusFlags : record {

    in_alarm:		bool;
    fault:		bool;
    overridden:		bool;
    out_of_service:	bool;

  };

  type BACnetObjectTypesSupported : record {

    analog_input: 			bool 	&optional;
    analog_output:			bool 	&optional;
    analog_value:			bool 	&optional;
    binary_input:			bool 	&optional;
    binary_output:			bool 	&optional;
    binary_value:			bool 	&optional;
    calendar:				bool 	&optional;
    command:				bool 	&optional;
    device:				bool 	&optional;
    event_enrollment:			bool 	&optional;
    file_:				bool 	&optional;
    group:				bool 	&optional;
    loop:				bool 	&optional;
    multi_state_input:			bool 	&optional;
    multi_state_output:			bool 	&optional;
    notification_class:			bool 	&optional;
    program:				bool 	&optional;
    schedule_:				bool 	&optional;
    averaging:				bool 	&optional;
    multi_state_value:			bool 	&optional;
    trend_log:				bool 	&optional;
    life_safety_point:			bool 	&optional;
    life_safety_zone:			bool 	&optional;
    accumulator:			bool 	&optional;
    pulse_converter:			bool 	&optional;
    event_log:				bool 	&optional;
    global_group:			bool 	&optional;
    trend_log_multiple:			bool 	&optional;
    load_control:			bool 	&optional;
    structured_view:			bool 	&optional;
    access_door:			bool 	&optional;
    access_credential:			bool 	&optional;
    access_point:			bool 	&optional;
    access_rights:			bool 	&optional;
    access_user:			bool 	&optional;
    access_zone:			bool 	&optional;
    credential_data_input:		bool 	&optional;
    network_security:			bool 	&optional;
    bitstring_value:			bool 	&optional;
    characterstring_value:		bool 	&optional;
    date_pattern_value:			bool 	&optional;
    date_value:				bool 	&optional;
    datetime_pattern_value:		bool 	&optional;
    datetime_value:			bool 	&optional;
    integer_value:			bool 	&optional;
    large_analog_value:			bool 	&optional;
    octetstring_value:			bool 	&optional;
    positive_integer_value:		bool 	&optional;
    time_pattern_value:			bool 	&optional;
    time_value:				bool 	&optional;
    notification_forwarder:		bool 	&optional;
    alert_enrollment:			bool 	&optional;
    channel:				bool 	&optional;
    lighting_output:			bool 	&optional;

  };

  type BACnetServicesSupported : record {

    acknowledgeAlarm:			bool	&optional;
    confirmedCOVNotification:		bool	&optional;
    confirmedEventNotification:		bool	&optional;
    getAlarmSummary:			bool	&optional;
    getEnrollmentSummary:		bool	&optional;
    subscribeCOV:			bool	&optional;
    atomicReadFile:			bool	&optional;
    atomicWriteFile:			bool	&optional;
    addListElement:			bool	&optional;
    removeListElement:			bool	&optional;
    createObject:			bool	&optional;
    deleteObject:			bool	&optional;
    readProperty:			bool	&optional;
    readPropertyConditional:		bool	&optional;
    readPropertyMultiple:		bool	&optional;
    writeProperty:			bool	&optional;
    writePropertyMultiple:		bool	&optional;
    deviceCommunicationControl:		bool	&optional;
    confirmedPrivateTransfer:		bool	&optional;
    confirmedTextMessage:		bool	&optional;
    reinitializeDevice:			bool	&optional;
    vtOpen: 				bool	&optional;
    vtClose:				bool	&optional;
    vtData: 				bool	&optional;
    authenticate:			bool	&optional;
    requestKey:				bool	&optional;
    i_Am:				bool	&optional;
    i_Have:				bool	&optional;
    unconfirmedCOVNotification:		bool	&optional;
    unconfirmedEventNotification:	bool	&optional;
    unconfirmedPrivateTransfer:		bool	&optional;
    unconfirmedTextMessage:		bool	&optional;
    timeSynchronization:		bool	&optional;
    who_Has:				bool	&optional;
    who_Is:				bool	&optional;
    readRange:				bool	&optional;
    utcTimeSynchronization:		bool	&optional;
    lifeSafetyOperation:		bool	&optional;
    subscribeCOVProperty: 		bool	&optional;
    getEventInformation: 		bool	&optional;
    writeGroup:				bool	&optional;

  };

}

export {

  global BACnetDeviceStatus: table[count] of string = {

    [0]		= "operational",
    [1]		= "operational-read-only",
    [2] 	= "download-required",
    [3]		= "download-in-progress",
    [4]		= "non-operational",
    [5] 	= "backup-in-progress"

  };

  global BACnetEventState: table[count] of string = {

    [0]		= "normal",
    [1]		= "fault",
    [2]		= "offnormal",
    [3]		= "high-limit",
    [4]		= "low-limit",
    [5]		= "life-safety-alarm"

  };

  global Error_Class : table[count] of string = {
  
    [0]		= "device",
    [1]		= "object_",
    [2]		= "property_",
    [3]		= "resources",
    [4]		= "security",
    [5]		= "services",
    [6]		= "vt",
    [7]		= "communication"

  };

  global Error_Code : table[count] of string = {

    [123] =	"abort-apdu-too-long",
    [124] =	"abort-application-exceeded-reply-time",
    [51] =	"abort-buffer-overflow",
    [135] =	"abort-insufficient-security",
    [52] =	"abort-invalid-apdu-in-this-state",
    [56] =	"abort-other",
    [125] =	"abort-out-of-resources",
    [53] =	"abort-preempted-by-higher-priority-task",
    [55] =	"abort-proprietary",
    [136] =	"abort-security-error",
    [54] =	"abort-segmentation-not-supported",
    [126] =	"abort-tsm-timeout",
    [127] =	"abort-window-size-out-of-range",
    [85] =	"access-denied",
    [115] =	"addressing-error",
    [86] =	"bad-destination-address",
    [87] =	"bad-destination-device-id",
    [88] =	"bad-signature",
    [89] =	"bad-source-address",
    [90] =	"bad-timestamp",
    [82] =	"busy",
    [91] =	"cannot-use-key",
    [92] =	"cannot-verify-message-id",
    [41] =	"character-set-not-supported",
    [83] =	"communication-disabled",
    [2] =	"configuration-in-progress",
    [93] =	"correct-key-revision",
    [43] =	"cov-subscription-failed",
    [47] =	"datatype-not-supported",
    [120] =	"delete-fdt-entry-failed",
    [3] =	"device-busy",
    [94] =	"destination-device-id-required",
    [121] =	"distribute-broadcast-failed",
    [95] =	"duplicate-message",
    [48] =	"duplicate-name",
    [49] =	"duplicate-object-id",
    [4] =	"dynamic-creation-not-supported",
    [96] =	"encryption-not-configured",
    [97] =	"encryption-required",
    [5] =	"file-access-denied",
    [128] =	"file-full",
    [129] =	"inconsistent-configuration",
    [130] =	"inconsistent-object-type",
    [7] =	"inconsistent-parameters",
    [8] =	"inconsistent-selection-criterion",
    [98] =	"incorrect-key",
    [131] =	"internal-error",
    [42] =	"invalid-array-index",
    [46] =	"invalid-configuration-data",
    [9] =	"invalid-data-type",
    [73] =	"invalid-event-state",
    [10] =	"invalid-file-access-method",
    [11] =	"invalid-file-start-position",
    [99] =	"invalid-key-data",
    [13] =	"invalid-parameter-data-type",
    [57] =	"invalid-tag",
    [14] =	"invalid-time-stamp",
    [100] =	"key-update-in-progress",
    [81] =	"list-element-not-found",
    [75] =	"log-buffer-full",
    [76] =	"logged-value-purged",
    [101] =	"malformed-message",
    [113] =	"message-too-long",
    [16] =	"missing-required-parameter",
    [58] =	"network-down",
    [74] =	"no-alarm-configured",
    [17] =	"no-objects-of-specified-type",
    [77] =	"no-property-specified",
    [18] =	"no-space-for-object",
    [19] =	"no-space-to-add-list-element",
    [20] =	"no-space-to-write-property",
    [21] =	"no-vt-sessions-available",
    [132] =	"not-configured",
    [78] =	"not-configured-for-triggered-logging",
    [44] =	"not-cov-property",
    [102] =	"not-key-server",
    [110] =	"not-router-to-dnet",
    [23] =	"object-deletion-not-permitted",
    [24] =	"object-identifier-already-exists",
    [0] =	"other",
    [25] =	"operational-problem",
    [45] =	"optional-functionality-not-supported",
    [133] =	"out-of-memory",
    [80] =	"parameter-out-of-range",
    [26] =	"password-failure",
    [22] =	"property-is-not-a-list",
    [50] =	"property-is-not-an-array",
    [27] =	"read-access-denied",
    [117] =	"read-bdt-failed",
    [119] =	"read-fdt-failed",
    [118] =	"register-foreign-device-failed",
    [59] =	"reject-buffer-overflow",
    [60] =	"reject-inconsistent-parameters",
    [61] =	"reject-invalid-parameter-data-type",
    [62] =	"reject-invalid-tag",
    [63] =	"reject-missing-required-parameter",
    [64] =	"reject-parameter-out-of-range",
    [65] =	"reject-too-many-arguments",
    [66] =	"reject-undefined-enumeration",
    [67] =	"reject-unrecognized-service",
    [68] =	"reject-proprietary",
    [69] =	"reject-other",
    [111] =	"router-busy",
    [114] =	"security-error",
    [103] =	"security-not-configured",
    [29] =	"service-request-denied",
    [104] =	"source-security-required",
    [84] =	"success",
    [30] =	"timeout",
    [105] =	"too-many-keys",
    [106] =	"unknown-authentication-type",
    [70] =	"unknown-device",
    [122] =	"unknown-file-size",
    [107] =	"unknown-key",
    [108] =	"unknown-key-revision",
    [112] =	"unknown-network-message",
    [31] =	"unknown-object",
    [32] =	"unknown-property",
    [79] =	"unknown-subscription",
    [71] =	"unknown-route",
    [109] =	"unknown-source-message",
    [34] =	"unknown-vt-class",
    [35] =	"unknown-vt-session",
    [36] =	"unsupported-object-type",
    [72] =	"value-not-initialized",
    [37] =	"value-out-of-range",
    [134] =	"value-too-long",
    [38] =	"vt-session-already-closed",
    [39] =	"vt-session-termination-failure",
    [40] =	"write-access-denied",
    [116] =	"write-bdt-failed",

  };

  global BACnetEventType: table[count] of string = {
  
    [0]		= "change-of-bitstring",
    [1]		= "change-of-state",
    [2]		= "change-of-value",
    [3]		= "command-failure",
    [4]		= "floating-limit",
    [5]		= "out-of-range",
    [6]		= "(deprecated) complex-event-type",
    [7]		= "(reserved)",
    [8]		= "change-of-life-safety",
    [9]		= "extended",
    [10]	= "buffer-ready",
    [11]	= "unsigned-range",
    [12]	= "(reserved)",
    [13]	= "access-event",
    [14]	= "double-out-of-range",
    [15]	= "signed-out-of-range",
    [16]	= "unsigned-out-of-range",
    [17]	= "change-of-characterstring",
    [18]	= "change-of-status-flags",
    [19]	= "change-of-reliability",
    [20]	= "none"

  };

  global BACnetObjectType: table[count] of string = {

    [0]		= "analog-input",
    [1]		= "analog-output",
    [2]		= "analog-value",
    [3]		= "binary-input",
    [4]		= "binary-output",
    [5]		= "binary-value",
    [6]		= "calendar",
    [7]		= "command",
    [8]		= "device",
    [9]		= "event-enrollment",
    [10]	= "file",
    [11]	= "group",
    [12]	= "loop",
    [13]	= "multi-state-input",
    [14]	= "multi-state-output",
    [15]	= "notification-class",
    [16]	= "program",
    [17]	= "schedule",
    [18]	= "averaging",
    [19]	= "multi-state-value",
    [20]	= "trend-log",
    [21]	= "life-safety-point",
    [22]	= "life-safety-zone",
    [23]	= "accumulator",
    [24]	= "pulse-converter",
    [25]	= "event-log",
    [26]	= "global-group",
    [27]	= "trend-log-multiple",
    [28]	= "load-control",
    [29]	= "structured-view",
    [30]	= "access-door",
    [31]	= "(unassigned)",
    [32]	= "access-credential",
    [33]	= "access-point",
    [34]	= "access-rights",
    [35]	= "access-user",
    [36]	= "access-zone",
    [37]	= "credential-data-input",
    [38]	= "network-security",
    [39]	= "bitstring-value",
    [40]	= "characterstring-value",
    [41]	= "date-pattern-value",
    [42]	= "date-value",
    [43]	= "datetime-pattern-value",
    [44]	= "datetime-value",
    [45]	= "integer-value",
    [46]	= "large-analog-value",
    [47]	= "octetstring-value",
    [48]	= "positive-integer-value",
    [49]	= "time-pattern-value",
    [50]	= "time-value",
    [51]	= "notification-forwarder",
    [52]	= "alert-enrollment",
    [53]	= "channel",
    [54]	= "lighting-output"

  };

  global BACnetNotifyType: table[count] of string = {
  
    [0]		= "alarm",
    [1]		= "event",
    [2]		= "ack-notification"

  };

  global BACnetSegmentation: table[count] of string = {

    [0] 	= "segmented-both",
    [1] 	= "segmented-transmit",
    [2]		= "segmented-receive",
    [3]		= "no-segmentation"

  };

  global BACnetVendorId: table[count] of string = {
    
    [0] =	"ASHRAE",
    [1] =	"NIST",
    [2] =	"The_Trane_Company",
    [3] =	"McQuay_International",
    [4] =	"PolarSoft",
    [5] =	"Johnson_Controls__Inc_",
    [6] =	"American_Auto-Matrix",
    [7] =	"Siemens_Schweiz_AG__Formerly__Landis___Staefa_Division_Europe_",
    [8] =	"Delta_Controls",
    [9] =	"Siemens_Schweiz_AG",
    [10] =	"Schneider_Electric",
    [11] =	"TAC",
    [12] =	"Orion_Analysis_Corporation",
    [13] =	"Teletrol_Systems_Inc_",
    [14] =	"Cimetrics_Technology",
    [15] =	"Cornell_University",
    [16] =	"United_Technologies_Carrier",
    [17] =	"Honeywell_Inc_",
    [18] =	"Alerton___Honeywell",
    [19] =	"TAC_AB",
    [20] =	"Hewlett-Packard_Company",
    [21] =	"Dorsettes_Inc_",
    [22] =	"Siemens_Schweiz_AG__Formerly__Cerberus_AG_",
    [23] =	"York_Controls_Group",
    [24] =	"Automated_Logic_Corporation",
    [25] =	"CSI_Control_Systems_International",
    [26] =	"Phoenix_Controls_Corporation",
    [27] =	"Innovex_Technologies__Inc_",
    [28] =	"KMC_Controls__Inc_",
    [29] =	"Xn_Technologies__Inc_",
    [30] =	"Hyundai_Information_Technology_Co___Ltd_",
    [31] =	"Tokimec_Inc_",
    [32] =	"Simplex",
    [33] =	"North_Building_Technologies_Limited",
    [34] =	"Notifier",
    [35] =	"Reliable_Controls_Corporation",
    [36] =	"Tridium_Inc_",
    [37] =	"Sierra_Monitor_Corporation_FieldServer_Technologies",
    [38] =	"Silicon_Energy",
    [39] =	"Kieback___Peter_GmbH___Co_KG",
    [40] =	"Anacon_Systems__Inc_",
    [41] =	"Systems_Controls___Instruments__LLC",
    [42] =	"Lithonia_Lighting",
    [43] =	"Micropower_Manufacturing",
    [44] =	"Matrix_Controls",
    [45] =	"METALAIRE",
    [46] =	"ESS_Engineering",
    [47] =	"Sphere_Systems_Pty_Ltd_",
    [48] =	"Walker_Technologies_Corporation",
    [49] =	"H_I_Solutions__Inc_",
    [50] =	"MBS_GmbH",
    [51] =	"SAMSON_AG",
    [52] =	"Badger_Meter_Inc_",
    [53] =	"DAIKIN_Industries_Ltd_",
    [54] =	"NARA_Controls_Inc_",
    [55] =	"Mammoth_Inc_",
    [56] =	"Liebert_Corporation",
    [57] =	"SEMCO_Incorporated",
    [58] =	"Air_Monitor_Corporation",
    [59] =	"TRIATEK__LLC",
    [60] =	"NexLight",
    [61] =	"Multistack",
    [62] =	"TSI_Incorporated",
    [63] =	"Weather-Rite__Inc_",
    [64] =	"Dunham-Bush",
    [65] =	"Reliance_Electric",
    [66] =	"LCS_Inc_",
    [67] =	"Regulator_Australia_PTY_Ltd_",
    [68] =	"Touch-Plate_Lighting_Controls",
    [69] =	"Amann_GmbH",
    [70] =	"RLE_Technologies",
    [71] =	"Cardkey_Systems",
    [72] =	"SECOM_Co___Ltd_",
    [73] =	"ABB_Gebudetechnik_AG_Bereich_NetServ",
    [74] =	"KNX_Association_cvba",
    [75] =	"Institute_of_Electrical_Installation_Engineers_of_Japan__IEIEJ_",
    [76] =	"Nohmi_Bosai__Ltd_",
    [77] =	"Carel_S_p_A_",
    [78] =	"AirSense_Technology__Inc_",
    [79] =	"Hochiki_Corporation",
    [80] =	"Fr__Sauter_AG",
    [81] =	"Matsushita_Electric_Works__Ltd_",
    [82] =	"Mitsubishi_Electric_Corporation__Inazawa_Works",
    [83] =	"Mitsubishi_Heavy_Industries__Ltd_",
    [84] =	"ITT_Bell___Gossett",
    [85] =	"Yamatake_Building_Systems_Co___Ltd_",
    [86] =	"The_Watt_Stopper__Inc_",
    [87] =	"Aichi_Tokei_Denki_Co___Ltd_",
    [88] =	"Activation_Technologies__LLC",
    [89] =	"Saia-Burgess_Controls__Ltd_",
    [90] =	"Hitachi__Ltd_",
    [91] =	"Novar_Corp__Trend_Control_Systems_Ltd_",
    [92] =	"Mitsubishi_Electric_Lighting_Corporation",
    [93] =	"Argus_Control_Systems__Ltd_",
    [94] =	"Kyuki_Corporation",
    [95] =	"Richards-Zeta_Building_Intelligence__Inc_",
    [96] =	"Scientech_R_D__Inc_",
    [97] =	"VCI_Controls__Inc_",
    [98] =	"Toshiba_Corporation",
    [99] =	"Mitsubishi_Electric_Corporation_Air_Conditioning___Refrigeration_Systems_Works",
    [100] =	"Custom_Mechanical_Equipment__LLC",
    [101] =	"ClimateMaster",
    [102] =	"ICP_Panel-Tec__Inc_",
    [103] =	"D-Tek_Controls",
    [104] =	"NEC_Engineering__Ltd_",
    [105] =	"PRIVA_BV",
    [106] =	"Meidensha_Corporation",
    [107] =	"JCI_Systems_Integration_Services",
    [108] =	"Freedom_Corporation",
    [109] =	"Neuberger_Gebudeautomation_GmbH",
    [110] =	"Sitronix",
    [111] =	"Leviton_Manufacturing",
    [112] =	"Fujitsu_Limited",
    [113] =	"Emerson_Network_Power",
    [114] =	"S__A__Armstrong__Ltd_",
    [115] =	"Visonet_AG",
    [116] =	"M_M_Systems__Inc_",
    [117] =	"Custom_Software_Engineering",
    [118] =	"Nittan_Company__Limited",
    [119] =	"Elutions_Inc___Wizcon_Systems_SAS_",
    [120] =	"Pacom_Systems_Pty___Ltd_",
    [121] =	"Unico__Inc_",
    [122] =	"Ebtron__Inc_",
    [123] =	"Scada_Engine",
    [124] =	"AC_Technology_Corporation",
    [125] =	"Eagle_Technology",
    [126] =	"Data_Aire__Inc_",
    [127] =	"ABB__Inc_",
    [128] =	"Transbit_Sp__z_o__o_",
    [129] =	"Toshiba_Carrier_Corporation",
    [130] =	"Shenzhen_Junzhi_Hi-Tech_Co___Ltd_",
    [131] =	"Tokai_Soft",
    [132] =	"Blue_Ridge_Technologies",
    [133] =	"Veris_Industries",
    [134] =	"Centaurus_Prime",
    [135] =	"Sand_Network_Systems",
    [136] =	"Regulvar__Inc_",
    [137] =	"AFDtek_Division_of_Fastek_International_Inc_",
    [138] =	"PowerCold_Comfort_Air_Solutions__Inc_",
    [139] =	"I_Controls",
    [140] =	"Viconics_Electronics__Inc_",
    [141] =	"Yaskawa_America__Inc_",
    [142] =	"DEOS_control_systems_GmbH",
    [143] =	"Digitale_Mess-_und_Steuersysteme_AG",
    [144] =	"Fujitsu_General_Limited",
    [145] =	"Project_Engineering_S_r_l_",
    [146] =	"Sanyo_Electric_Co___Ltd_",
    [147] =	"Integrated_Information_Systems__Inc_",
    [148] =	"Temco_Controls__Ltd_",
    [149] =	"Airtek_International_Inc_",
    [150] =	"Advantech_Corporation",
    [151] =	"Titan_Products__Ltd_",
    [152] =	"Regel_Partners",
    [153] =	"National_Environmental_Product",
    [154] =	"Unitec_Corporation",
    [155] =	"Kanden_Engineering_Company",
    [156] =	"Messner_Gebudetechnik_GmbH",
    [157] =	"Integrated_CH",
    [158] =	"Price_Industries",
    [159] =	"SE-Elektronic_GmbH",
    [160] =	"Rockwell_Automation",
    [161] =	"Enflex_Corp_",
    [162] =	"ASI_Controls",
    [163] =	"SysMik_GmbH_Dresden",
    [164] =	"HSC_Regelungstechnik_GmbH",
    [165] =	"Smart_Temp_Australia_Pty___Ltd_",
    [166] =	"Cooper_Controls",
    [167] =	"Duksan_Mecasys_Co___Ltd_",
    [168] =	"Fuji_IT_Co___Ltd_",
    [169] =	"Vacon_Plc",
    [170] =	"Leader_Controls",
    [171] =	"Cylon_Controls__Ltd_",
    [172] =	"Compas",
    [173] =	"Mitsubishi_Electric_Building_Techno-Service_Co___Ltd_",
    [174] =	"Building_Control_Integrators",
    [175] =	"ITG_Worldwide__M__Sdn_Bhd",
    [176] =	"Lutron_Electronics_Co___Inc_",
    [178] =	"LOYTEC_Electronics_GmbH",
    [179] =	"ProLon",
    [180] =	"Mega_Controls_Limited",
    [181] =	"Micro_Control_Systems__Inc_",
    [182] =	"Kiyon__Inc_",
    [183] =	"Dust_Networks",
    [184] =	"Advanced_Building_Automation_Systems",
    [185] =	"Hermos_AG",
    [186] =	"CEZIM",
    [187] =	"Softing",
    [188] =	"Lynxspring",
    [189] =	"Schneider_Toshiba_Inverter_Europe",
    [190] =	"Danfoss_Drives_A_S",
    [191] =	"Eaton_Corporation",
    [192] =	"Matyca_S_A_",
    [193] =	"Botech_AB",
    [194] =	"Noveo__Inc_",
    [195] =	"AMEV",
    [196] =	"Yokogawa_Electric_Corporation",
    [197] =	"GFR_Gesellschaft_fr_Regelungstechnik",
    [198] =	"Exact_Logic",
    [199] =	"Mass_Electronics_Pty_Ltd_dba_Innotech_Control_Systems_Australia",
    [200] =	"Kandenko_Co___Ltd_",
    [201] =	"DTF__Daten-Technik_Fries",
    [202] =	"Klimasoft__Ltd_",
    [203] =	"Toshiba_Schneider_Inverter_Corporation",
    [204] =	"Control_Applications__Ltd_",
    [205] =	"KDT_Systems_Co___Ltd_",
    [206] =	"Onicon_Incorporated",
    [207] =	"Automation_Displays__Inc_",
    [208] =	"Control_Solutions__Inc_",
    [209] =	"Remsdaq_Limited",
    [210] =	"NTT_Facilities__Inc_",
    [211] =	"VIPA_GmbH",
    [212] =	"TSC21_Association_of_Japan",
    [213] =	"Strato_Automation",
    [214] =	"HRW_Limited",
    [215] =	"Lighting_Control___Design__Inc_",
    [216] =	"Mercy_Electronic_and_Electrical_Industries",
    [217] =	"Samsung_SDS_Co___Ltd",
    [218] =	"Impact_Facility_Solutions__Inc_",
    [219] =	"Aircuity",
    [220] =	"Control_Techniques__Ltd_",
    [221] =	"OpenGeneral_Pty___Ltd_",
    [222] =	"WAGO_Kontakttechnik_GmbH___Co__KG",
    [223] =	"Cerus_Industrial",
    [224] =	"Chloride_Power_Protection_Company",
    [225] =	"Computrols__Inc_",
    [226] =	"Phoenix_Contact_GmbH___Co__KG",
    [227] =	"Grundfos_Management_A_S",
    [228] =	"Ridder_Drive_Systems",
    [229] =	"Soft_Device_SDN_BHD",
    [230] =	"Integrated_Control_Technology_Limited",
    [231] =	"AIRxpert_Systems__Inc_",
    [232] =	"Microtrol_Limited",
    [233] =	"Red_Lion_Controls",
    [234] =	"Digital_Electronics_Corporation",
    [235] =	"Ennovatis_GmbH",
    [236] =	"Serotonin_Software_Technologies__Inc_",
    [237] =	"LS_Industrial_Systems_Co___Ltd_",
    [238] =	"Square_D_Company",
    [239] =	"S_Squared_Innovations__Inc_",
    [240] =	"Aricent_Ltd_",
    [241] =	"EtherMetrics__LLC",
    [242] =	"Industrial_Control_Communications__Inc_",
    [243] =	"Paragon_Controls__Inc_",
    [244] =	"A__O__Smith_Corporation",
    [245] =	"Contemporary_Control_Systems__Inc_",
    [246] =	"Intesis_Software_SL",
    [247] =	"Ingenieurgesellschaft_N__Hartleb_mbH",
    [248] =	"Heat-Timer_Corporation",
    [249] =	"Ingrasys_Technology__Inc_",
    [250] =	"Costerm_Building_Automation",
    [251] =	"WILO_SE",
    [252] =	"Embedia_Technologies_Corp_",
    [253] =	"Technilog",
    [254] =	"HR_Controls_Ltd____Co__KG",
    [255] =	"Lennox_International__Inc_",
    [256] =	"RK-Tec_Rauchklappen-Steuerungssysteme_GmbH___Co__KG",
    [257] =	"Thermomax__Ltd_",
    [258] =	"ELCON_Electronic_Control__Ltd_",
    [259] =	"Larmia_Control_AB",
    [260] =	"BACnet_Stack_at_SourceForge",
    [261] =	"G4S_Security_Services_A_S",
    [262] =	"Exor_International_S_p_A_",
    [263] =	"Cristal_Controles",
    [264] =	"Regin_AB",
    [265] =	"Dimension_Software__Inc__",
    [266] =	"SynapSense_Corporation",
    [267] =	"Beijing_Nantree_Electronic_Co___Ltd_",
    [268] =	"Camus_Hydronics_Ltd_",
    [269] =	"Kawasaki_Heavy_Industries__Ltd__",
    [270] =	"Critical_Environment_Technologies",
    [271] =	"ILSHIN_IBS_Co___Ltd_",
    [272] =	"ELESTA_Energy_Control_AG",
    [273] =	"KROPMAN_Installatietechniek",
    [274] =	"Baldor_Electric_Company",
    [275] =	"INGA_mbH",
    [276] =	"GE_Consumer___Industrial",
    [277] =	"Functional_Devices__Inc_",
    [278] =	"ESAC",
    [279] =	"M-System_Co___Ltd_",
    [280] =	"Yokota_Co___Ltd_",
    [281] =	"Hitranse_Technology_Co___LTD",
    [282] =	"Federspiel_Controls",
    [283] =	"Kele__Inc_",
    [284] =	"Opera_Electronics__Inc_",
    [285] =	"Gentec",
    [286] =	"Embedded_Science_Labs__LLC",
    [287] =	"Parker_Hannifin_Corporation",
    [288] =	"MaCaPS_International_Limited",
    [289] =	"Link4_Corporation",
    [290] =	"Romutec_Steuer-u__Regelsysteme_GmbH_",
    [291] =	"Pribusin__Inc_",
    [292] =	"Advantage_Controls",
    [293] =	"Critical_Room_Control",
    [294] =	"LEGRAND",
    [295] =	"Tongdy_Control_Technology_Co___Ltd__",
    [296] =	"ISSARO_Integrierte_Systemtechnik",
    [297] =	"Pro-Dev_Industries",
    [298] =	"DRI-STEEM",
    [299] =	"Creative_Electronic_GmbH",
    [300] =	"Swegon_AB",
    [301] =	"Jan_Brachacek",
    [302] =	"Hitachi_Appliances__Inc_",
    [303] =	"Real_Time_Automation__Inc_",
    [304] =	"ITEC_Hankyu-Hanshin_Co_",
    [305] =	"Cyrus_E_M_Engineering_Co___Ltd__",
    [306] =	"Racine_Federated__Inc_",
    [307] =	"Cirrascale_Corporation",
    [308] =	"Elesta_GmbH_Building_Automation_",
    [309] =	"Securiton",
    [310] =	"OSlsoft__Inc_",
    [311] =	"Hanazeder_Electronic_GmbH_",
    [312] =	"Honeywell_Security_Deutschland__Novar_GmbH",
    [313] =	"Siemens_Energy___Automation__Inc_",
    [314] =	"ETM_Professional_Control_GmbH",
    [315] =	"Meitav-tec__Ltd_",
    [316] =	"Janitza_Electronics_GmbH_",
    [317] =	"MKS_Nordhausen",
    [318] =	"De_Gier_Drive_Systems_B_V__",
    [319] =	"Cypress_Envirosystems",
    [320] =	"SMARTron_s_r_o_",
    [321] =	"Verari_Systems__Inc_",
    [322] =	"K-W_Electronic_Service__Inc_",
    [323] =	"ALFA-SMART_Energy_Management",
    [324] =	"Telkonet__Inc_",
    [325] =	"Securiton_GmbH",
    [326] =	"Cemtrex__Inc_",
    [327] =	"Performance_Technologies__Inc_",
    [328] =	"Xtralis__Aust__Pty_Ltd",
    [329] =	"TROX_GmbH",
    [330] =	"Beijing_Hysine_Technology_Co___Ltd",
    [331] =	"RCK_Controls__Inc_",
    [332] =	"Distech_Controls_SAS",
    [333] =	"Novar_Honeywell",
    [334] =	"The_S4_Group__Inc_",
    [335] =	"Schneider_Electric",
    [336] =	"LHA_Systems",
    [337] =	"GHM_engineering_Group__Inc_",
    [338] =	"Cllimalux_S_A_",
    [339] =	"VAISALA_Oyj",
    [340] =	"COMPLEX__Beijing__Technology__Co___LTD_",
    [341] =	"SCADAmetrics",
    [342] =	"POWERPEG_NSI_Limited",
    [343] =	"BACnet_Interoperability_Testing_Services__Inc_",
    [344] =	"Teco_a_s_",
    [345] =	"Plexus_Technology__Inc_",
    [346] =	"Energy_Focus__Inc_",
    [347] =	"Powersmiths_International_Corp_",
    [348] =	"Nichibei_Co___Ltd_",
    [349] =	"HKC_Technology_Ltd_",
    [350] =	"Ovation_Networks__Inc_",
    [351] =	"Setra_Systems",
    [352] =	"AVG_Automation",
    [353] =	"ZXC_Ltd_",
    [354] =	"Byte_Sphere",
    [355] =	"Generiton_Co___Ltd_",
    [356] =	"Holter_Regelarmaturen_GmbH___Co__KG",
    [357] =	"Bedford_Instruments__LLC",
    [358] =	"Standair_Inc_",
    [359] =	"WEG_Automation_-_R_D",
    [360] =	"Prolon_Control_Systems_ApS",
    [361] =	"Inneasoft",
    [362] =	"ConneXSoft_GmbH",
    [363] =	"CEAG_Notlichtsysteme_GmbH",
    [364] =	"Distech_Controls_Inc_",
    [365] =	"Industrial_Technology_Research_Institute",
    [366] =	"ICONICS__Inc_",
    [367] =	"IQ_Controls_s_c_",
    [368] =	"OJ_Electronics_A_S",
    [369] =	"Rolbit_Ltd_",
    [370] =	"Synapsys_Solutions_Ltd_",
    [371] =	"ACME_Engineering_Prod__Ltd_",
    [372] =	"Zener_Electric_Pty__Ltd_",
    [373] =	"Selectronix__Inc_",
    [374] =	"Gorbet___Banerjee__LLC_",
    [375] =	"IME",
    [376] =	"Stephen_H__Dawson_Computer_Service",
    [377] =	"Accutrol__LLC",
    [378] =	"Schneider_Elektronik_GmbH",
    [379] =	"Alpha-Inno_Tec_GmbH",
    [380] =	"ADMMicro__Inc_",
    [381] =	"Greystone_Energy_Systems__Inc_",
    [382] =	"CAP_Technologie",
    [383] =	"KeRo_Systems",
    [384] =	"Domat_Control_System_s_r_o_",
    [385] =	"Efektronics_Pty__Ltd_",
    [386] =	"Hekatron_Vertriebs_GmbH",
    [387] =	"Securiton_AG",
    [388] =	"Carlo_Gavazzi_Controls_SpA",
    [389] =	"Chipkin_Automation_Systems",
    [390] =	"Savant_Systems__LLC",
    [391] =	"Simmtronic_Lighting_Controls",
    [392] =	"Abelko_Innovation_AB",
    [393] =	"Seresco_Technologies_Inc_",
    [394] =	"IT_Watchdogs",
    [395] =	"Automation_Assist_Japan_Corp_",
    [396] =	"Thermokon_Sensortechnik_GmbH",
    [397] =	"EGauge_Systems__LLC",
    [398] =	"Quantum_Automation__ASIA__PTE__Ltd_",
    [399] =	"Toshiba_Lighting___Technology_Corp_",
    [400] =	"SPIN_Engenharia_de_Automao_Ltda_",
    [401] =	"Logistics_Systems___Software_Services_India_PVT__Ltd_",
    [402] =	"Delta_Controls_Integration_Products",
    [403] =	"Focus_Media",
    [404] =	"LUMEnergi_Inc_",
    [405] =	"Kara_Systems",
    [406] =	"RF_Code__Inc_",
    [407] =	"Fatek_Automation_Corp_",
    [408] =	"JANDA_Software_Company__LLC",
    [409] =	"Open_System_Solutions_Limited",
    [410] =	"Intelec_Systems_PTY_Ltd_",
    [411] =	"Ecolodgix__LLC",
    [412] =	"Douglas_Lighting_Controls",
    [413] =	"iSAtech_GmbH",
    [414] =	"AREAL",
    [415] =	"Beckhoff_Automation_GmbH",
    [416] =	"IPAS_GmbH",
    [417] =	"KE2_Therm_Solutions",
    [418] =	"Base2Products",
    [419] =	"DTL_Controls__LLC",
    [420] =	"INNCOM_International__Inc_",
    [421] =	"BTR_Netcom_GmbH",
    [422] =	"Greentrol_Automation__Inc",
    [423] =	"BELIMO_Automation_AG",
    [424] =	"Samsung_Heavy_Industries_Co__Ltd",
    [425] =	"Triacta_Power_Technologies__Inc_",
    [426] =	"Globestar_Systems",
    [427] =	"MLB_Advanced_Media__LP",
    [428] =	"SWG_Stuckmann_Wirtschaftliche_Gebudesysteme_GmbH",
    [429] =	"SensorSwitch",
    [430] =	"Multitek_Power_Limited",
    [431] =	"Aquametro_AG",
    [432] =	"LG_Electronics_Inc_",
    [433] =	"Electronic_Theatre_Controls__Inc_",
    [434] =	"Mitsubishi_Electric_Corporation_Nagoya_Works",
    [435] =	"Delta_Electronics__Inc_",
    [436] =	"Elma_Kurtalj__Ltd_",
    [437] =	"ADT_Fire_and_Security_Sp__A_o_o_",
    [438] =	"Nedap_Security_Management",
    [439] =	"ESC_Automation_Inc_",
    [440] =	"DSP4YOU_Ltd_",
    [441] =	"GE_Sensing_and_Inspection_Technologies",
    [442] =	"Embedded_Systems_SIA",
    [443] =	"BEFEGA_GmbH",
    [444] =	"Baseline_Inc_",
    [445] =	"M2M_Systems_Integrators",
    [446] =	"OEMCtrl",
    [447] =	"Clarkson_Controls_Limited",
    [448] =	"Rogerwell_Control_System_Limited",
    [449] =	"SCL_Elements",
    [450] =	"Hitachi_Ltd_",
    [451] =	"Newron_System_SA",
    [452] =	"BEVECO_Gebouwautomatisering_BV",
    [453] =	"Streamside_Solutions",
    [454] =	"Yellowstone_Soft",
    [455] =	"Oztech_Intelligent_Systems_Pty_Ltd_",
    [456] =	"Novelan_GmbH",
    [457] =	"Flexim_Americas_Corporation",
    [458] =	"ICP_DAS_Co___Ltd_",
    [459] =	"CARMA_Industries_Inc_",
    [460] =	"Log-One_Ltd_",
    [461] =	"TECO_Electric___Machinery_Co___Ltd_",
    [462] =	"ConnectEx__Inc_",
    [463] =	"Turbo_DDC_Sdwest",
    [464] =	"Quatrosense_Environmental_Ltd_",
    [465] =	"Fifth_Light_Technology_Ltd_",
    [466] =	"Scientific_Solutions__Ltd_",
    [467] =	"Controller_Area_Network_Solutions__M__Sdn_Bhd",
    [468] =	"RESOL_-_Elektronische_Regelungen_GmbH",
    [469] =	"RPBUS_LLC",
    [470] =	"BRS_Sistemas_Eletronicos",
    [471] =	"WindowMaster_A_S",
    [472] =	"Sunlux_Technologies_Ltd_",
    [473] =	"Measurlogic",
    [474] =	"Frimat_GmbH",
    [475] =	"Spirax_Sarco",
    [476] =	"Luxtron",
    [477] =	"Raypak_Inc",
    [478] =	"Air_Monitor_Corporation",
    [479] =	"Regler_Och_Webbteknik_Sverige__ROWS_",
    [480] =	"Intelligent_Lighting_Controls_Inc_",
    [481] =	"Sanyo_Electric_Industry_Co___Ltd",
    [482] =	"E-Mon_Energy_Monitoring_Products",
    [483] =	"Digital_Control_Systems",
    [484] =	"ATI_Airtest_Technologies__Inc_",
    [485] =	"SCS_SA",
    [486] =	"HMS_Industrial_Networks_AB",
    [487] =	"Shenzhen_Universal_Intellisys_Co_Ltd",
    [488] =	"EK_Intellisys_Sdn_Bhd",
    [489] =	"SysCom",
    [490] =	"Firecom__Inc_",
    [491] =	"ESA_Elektroschaltanlagen_Grimma_GmbH",
    [492] =	"Kumahira_Co_Ltd",
    [493] =	"Hotraco",
    [494] =	"SABO_Elektronik_GmbH",
    [495] =	"Equip_Trans",
    [496] =	"TCS_Basys_Controls",
    [497] =	"FlowCon_International_A_S",
    [498] =	"ThyssenKrupp_Elevator_Americas",
    [499] =	"Abatement_Technologies",
    [500] =	"Continental_Control_Systems__LLC",
    [501] =	"WISAG_Automatisierungstechnik_GmbH___Co_KG",
    [502] =	"EasyIO",
    [503] =	"EAP-Electric_GmbH",
    [504] =	"Hardmeier",
    [505] =	"Mircom_Group_of_Companies",
    [506] =	"Quest_Controls",
    [507] =	"Mestek__Inc",
    [508] =	"Pulse_Energy",
    [509] =	"Tachikawa_Corporation",
    [510] =	"University_of_Nebraska-Lincoln",
    [511] =	"Redwood_Systems",
    [512] =	"PASStec_Industrie-Elektronik_GmbH",
    [513] =	"NgEK__Inc_",
    [514] =	"FAW_Electronics_Ltd",
    [515] =	"Jireh_Energy_Tech_Co___Ltd_",
    [516] =	"Enlighted_Inc_",
    [517] =	"El-Piast_Sp__Z_o_o",
    [518] =	"NetxAutomation_Software_GmbH",
    [519] =	"Invertek_Drives",
    [520] =	"Deutschmann_Automation_GmbH___Co__KG",
    [521] =	"EMU_Electronic_AG",
    [522] =	"Phaedrus_Limited",
    [523] =	"Sigmatek_GmbH___Co_KG",
    [524] =	"Marlin_Controls",
    [525] =	"Circutor__SA",
    [526] =	"UTC_Fire___Security",
    [527] =	"DENT_Instruments__Inc_",
    [528] =	"FHP_Manufacturing_Company_-_Bosch_Group",
    [529] =	"GE_Intelligent_Platforms",
    [530] =	"Inner_Range_Pty_Ltd",
    [531] =	"GLAS_Energy_Technology",
    [532] =	"MSR-Electronic-GmbH",
    [533] =	"Energy_Control_Systems__Inc_",
    [534] =	"EMT_Controls",
    [535] =	"Daintree_Networks_Inc_",
    [536] =	"EURO_ICC_d_o_o",
    [537] =	"TE_Connectivity_Energy",
    [538] =	"GEZE_GmbH",
    [539] =	"NEC_Corporation",
    [540] =	"Ho_Cheung_International_Company_Limited",
    [541] =	"Sharp_Manufacturing_Systems_Corporation",
    [542] =	"DOT_CONTROLS_a_s_",
    [543] =	"BeaconMeds",
    [544] =	"Midea_Commercial_Aircon",
    [545] =	"WattMaster_Controls",
    [546] =	"Kamstrup_A_S",
    [547] =	"CA_Computer_Automation_GmbH",
    [548] =	"Laars_Heating_Systems_Company",
    [549] =	"Hitachi_Systems__Ltd_",
    [550] =	"Fushan_AKE_Electronic_Engineering_Co___Ltd_",
    [551] =	"Toshiba_International_Corporation",
    [552] =	"Starman_Systems__LLC",
    [553] =	"Samsung_Techwin_Co___Ltd_",
    [554] =	"ISAS-Integrated_Switchgear_and_Systems_P_L",
    [556] =	"Obvius",
    [557] =	"Marek_Guzik",
    [558] =	"Vortek_Instruments__LLC",
    [559] =	"Universal_Lighting_Technologies",
    [560] =	"Myers_Power_Products__Inc_",
    [561] =	"Vector_Controls_GmbH",
    [562] =	"Crestron_Electronics__Inc_",
    [563] =	"A_E_Controls_Limited",
    [564] =	"Projektomontaza_A_D_",
    [565] =	"Freeaire_Refrigeration",
    [566] =	"Aqua_Cooler_Pty_Limited",
    [567] =	"Basic_Controls",
    [568] =	"GE_Measurement_and_Control_Solutions_Advanced_Sensors",
    [569] =	"EQUAL_Networks",
    [570] =	"Millennial_Net",
    [571] =	"APLI_Ltd",
    [572] =	"Electro_Industries_GaugeTech",
    [573] =	"SangMyung_University",
    [574] =	"Coppertree_Analytics__Inc_",
    [575] =	"CoreNetiX_GmbH",
    [576] =	"Acutherm",
    [577] =	"Dr__Riedel_Automatisierungstechnik_GmbH",
    [578] =	"Shina_System_Co___Ltd",
    [579] =	"Iqapertus",
    [580] =	"PSE_Technology",
    [581] =	"BA_Systems",
    [582] =	"BTICINO",
    [583] =	"Monico__Inc_",
    [584] =	"iCue",
    [585] =	"tekmar_Control_Systems_Ltd_",
    [586] =	"Control_Technology_Corporation",
    [587] =	"GFAE_GmbH",
    [588] =	"BeKa_Software_GmbH",
    [589] =	"Isoil_Industria_SpA",
    [590] =	"Home_Systems_Consulting_SpA",
    [591] =	"Socomec",
    [592] =	"Everex_Communications__Inc_",
    [593] =	"Ceiec_Electric_Technology",
    [594] =	"Atrila_GmbH",
    [595] =	"WingTechs",
    [596] =	"Shenzhen_Mek_Intellisys_Pte_Ltd_",
    [597] =	"Nestfield_Co___Ltd_",
    [598] =	"Swissphone_Telecom_AG",
    [599] =	"PNTECH_JSC",
    [600] =	"Horner_APG__LLC",
    [601] =	"PVI_Industries__LLC",
    [602] =	"Ela-compil",
    [603] =	"Pegasus_Automation_International_LLC",
    [604] =	"Wight_Electronic_Services_Ltd_",
    [605] =	"Marcom",
    [606] =	"Exhausto_A_S",
    [607] =	"Dwyer_Instruments__Inc_",
    [608] =	"Link_GmbH",
    [609] =	"Oppermann_Regelgerate_GmbH",
    [610] =	"NuAire__Inc_",
    [611] =	"Nortec_Humidity__Inc_",
    [612] =	"Bigwood_Systems__Inc_",
    [613] =	"Enbala_Power_Networks",
    [614] =	"Inter_Energy_Co___Ltd_",
    [615] =	"ETC",
    [616] =	"COMELEC_S_A_R_L",
    [617] =	"Pythia_Technologies",
    [618] =	"TrendPoint_Systems__Inc_",
    [619] =	"AWEX",
    [620] =	"Eurevia",
    [621] =	"Kongsberg_E-lon_AS",
    [622] =	"FlaktWoods",
    [623] =	"E___E_Elektronik_GES_M_B_H_",
    [624] =	"ARC_Informatique",
    [625] =	"SKIDATA_AG",
    [626] =	"WSW_Solutions",
    [627] =	"Trefon_Electronic_GmbH",
    [628] =	"Dongseo_System",
    [629] =	"Kanontec_Intelligence_Technology_Co___Ltd_",
    [630] =	"EVCO_S_p_A_",
    [631] =	"Accuenergy__CANADA__Inc_",
    [632] =	"SoftDEL",
    [633] =	"Orion_Energy_Systems__Inc_",
    [634] =	"Roboticsware",
    [635] =	"DOMIQ_Sp__z_o_o_",
    [636] =	"Solidyne",
    [637] =	"Elecsys_Corporation",
    [638] =	"Conditionaire_International_Pty__Limited",
    [639] =	"Quebec__Inc_",
    [640] =	"Homerun_Holdings",
    [641] =	"RFM__Inc_",
    [642] =	"Comptek",
    [643] =	"Westco_Systems__Inc_",
    [644] =	"Advancis_Software___Services_GmbH",
    [645] =	"Intergrid__LLC",
    [646] =	"Markerr_Controls__Inc_",
    [647] =	"Toshiba_Elevator_and_Building_Systems_Corporation",
    [648] =	"Spectrum_Controls__Inc_",
    [649] =	"Mkservice",
    [650] =	"Fox_Thermal_Instruments",
    [651] =	"SyxthSense_Ltd",
    [652] =	"DUHA_System_S_R_O_",
    [653] =	"NIBE",
    [654] =	"Melink_Corporation",
    [655] =	"Fritz-Haber-Institut",
    [656] =	"MTU_Onsite_Energy_GmbH__Gas_Power_Systems",
    [657] =	"Omega_Engineering__Inc_",
    [658] =	"Avelon",
    [659] =	"Ywire_Technologies__Inc_",
    [660] =	"M_R__Engineering_Co___Ltd_",
    [661] =	"Lochinvar__LLC",
    [662] =	"Sontay_Limited",
    [663] =	"GRUPA_Slawomir_Chelminski",
    [664] =	"Arch_Meter_Corporation",
    [665] =	"Senva__Inc_",
    [667] =	"FM-Tec",
    [668] =	"Systems_Specialists__Inc_",
    [669] =	"SenseAir",
    [670] =	"AB_IndustrieTechnik_Srl",
    [671] =	"Cortland_Research__LLC",
    [672] =	"MediaView",
    [673] =	"VDA_Elettronica",
    [674] =	"CSS__Inc_",
    [675] =	"Tek-Air_Systems__Inc_",
    [676] =	"ICDT",
    [677] =	"The_Armstrong_Monitoring_Corporation",
    [678] =	"DIXELL_S_r_l",
    [679] =	"Lead_System__Inc_",
    [680] =	"ISM_EuroCenter_S_A_",
    [681] =	"TDIS",
    [682] =	"Trade_FIDES",
    [683] =	"Knrr_GmbH__Emerson_Network_Power_",
    [684] =	"Resource_Data_Management",
    [685] =	"Abies_Technology__Inc_",
    [686] =	"Amalva",
    [687] =	"MIRAE_Electrical_Mfg__Co___Ltd_",
    [688] =	"HunterDouglas_Architectural_Projects_Scandinavia_ApS",
    [689] =	"RUNPAQ_Group_Co___Ltd",
    [690] =	"Unicard_SA",
    [691] =	"IE_Technologies",
    [692] =	"Ruskin_Manufacturing",
    [693] =	"Calon_Associates_Limited",
    [694] =	"Contec_Co___Ltd_",
    [695] =	"iT_GmbH",
    [696] =	"Autani_Corporation",
    [697] =	"Christian_Fortin",
    [698] =	"HDL",
    [699] =	"IPID_Sp__Z_O_O_Limited",
    [700] =	"Fuji_Electric_Co___Ltd",
    [701] =	"View__Inc_",
    [702] =	"Samsung_S1_Corporation",
    [703] =	"New_Lift",
    [704] =	"VRT_Systems",
    [705] =	"Motion_Control_Engineering__Inc_",
    [706] =	"Weiss_Klimatechnik_GmbH",
    [707] =	"Elkon",
    [708] =	"Eliwell_Controls_S_r_l_",
    [709] =	"Japan_Computer_Technos_Corp",
    [710] =	"Rational_Network_ehf",
    [711] =	"Magnum_Energy_Solutions__LLC",
    [712] =	"MelRok",
    [713] =	"VAE_Group",
    [714] =	"LGCNS",
    [715] =	"Berghof_Automationstechnik_GmbH",
    [716] =	"Quark_Communications__Inc_",
    [717] =	"Sontex",
    [718] =	"mivune_AG",
    [719] =	"Panduit",
    [720] =	"Smart_Controls__LLC",
    [721] =	"Compu-Aire__Inc_",
    [722] =	"Sierra",
    [723] =	"ProtoSense_Technologies",
    [724] =	"Eltrac_Technologies_Pvt_Ltd",
    [725] =	"Bektas_Invisible_Controls_GmbH",
    [726] =	"Entelec"
  
  };

}
