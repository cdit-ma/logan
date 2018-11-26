-- ****************** SqlDBM: Modified for postgres ******************;
-- ***************************************************;

DROP TABLE PortLifecycleEvent;


DROP TABLE WorkloadEvent;


DROP TABLE PortEvent;


DROP TABLE ComponentLifecycleEvent;


DROP TABLE UserEvent;


DROP TABLE WorkerInstance;


DROP TABLE Port;


DROP TABLE ComponentInstance;


DROP TABLE Hardware.SystemStatus;


DROP TABLE Hardware.ProcessStatus;


DROP TABLE Hardware.InterfaceStatus;


DROP TABLE Hardware.FilesystemStatus;


DROP TABLE Hardware.System;


DROP TABLE Hardware.CPUStatus;


DROP TABLE Hardware.Process;


DROP TABLE Hardware.Interface;


DROP TABLE Hardware.Filesystem;


DROP TABLE Container;


DROP TABLE Worker;


DROP TABLE Component;


DROP TABLE Node;


DROP TABLE ExperimentRun;


DROP TABLE Experiment;


DROP SCHEMA Hardware;


CREATE SCHEMA Hardware;

-- ************************************** Experiment

CREATE TABLE Experiment
(
 Name         TEXT NOT NULL ,
 ExperimentID SERIAL ,
 ModelName    TEXT NOT NULL ,
 Metadata     JSON ,

PRIMARY KEY (ExperimentID),
CONSTRAINT UniqueExperimentName UNIQUE (Name)
);






-- ************************************** ExperimentRun

CREATE TABLE ExperimentRun
(
 ExperimentID    INT NOT NULL ,
 ExperimentRunID SERIAL ,
 JobNum          INT NOT NULL ,
 StartTime       TIMESTAMP NOT NULL ,
 EndTime         TIMESTAMP ,
 Metadata        JSON ,

PRIMARY KEY (ExperimentRunID),
CONSTRAINT UniqueJobNumPerExperiment UNIQUE (ExperimentID, JobNum),
CONSTRAINT FK_Job_ExperimentID_Experiment_ExperimentID FOREIGN KEY (ExperimentID) REFERENCES Experiment (ExperimentID)
);






-- ************************************** Worker

CREATE TABLE Worker
(
 WorkerID        SERIAL ,
 Name            TEXT NOT NULL ,
 ExperimentRunID INT NOT NULL ,
 GraphmlID       TEXT NOT NULL ,

PRIMARY KEY (WorkerID),
CONSTRAINT UniqueWorkerName UNIQUE (Name),
CONSTRAINT FK_420 FOREIGN KEY (ExperimentRunID) REFERENCES ExperimentRun (ExperimentRunID)
);






-- ************************************** Component

CREATE TABLE Component
(
 ComponentID     SERIAL ,
 Name            TEXT NOT NULL ,
 ExperimentRunID INT NOT NULL ,
 GraphmlID       TEXT NOT NULL ,

PRIMARY KEY (ComponentID),
CONSTRAINT UniqueComponentNamePerRun UNIQUE (Name, ExperimentRunID),
CONSTRAINT FK_403 FOREIGN KEY (ExperimentRunID) REFERENCES ExperimentRun (ExperimentRunID)
);






-- ************************************** Node

CREATE TABLE Node
(
 NodeID          SERIAL ,
 Hostname        TEXT NOT NULL ,
 IP              INET NOT NULL ,
 ExperimentRunID INT NOT NULL ,
 GraphmlID       TEXT NOT NULL ,

PRIMARY KEY (NodeID),
CONSTRAINT UniqueIP UNIQUE (IP, ExperimentRunID),
CONSTRAINT FK_360 FOREIGN KEY (ExperimentRunID) REFERENCES ExperimentRun (ExperimentRunID)
);








-- ************************************** Cluster

CREATE TABLE Container
(
 ContainerID       SERIAL ,
 NodeID          INT NOT NULL ,
 Name            TEXT NOT NULL ,
 GraphmlID       TEXT NOT NULL,
 Type            TEXT NOT NULL,

PRIMARY KEY (ContainerID),
CONSTRAINT UniqueGraphmlIDPerRun UNIQUE (NodeID, GraphmlID),
CONSTRAINT FK_356 FOREIGN KEY (NodeID) REFERENCES Node (NodeID)
);






-- ************************************** Hardware.System

CREATE TABLE Hardware.System
(
 SystemID     SERIAL ,
 SampleTime     TIMESTAMP NOT NULL ,
 OSName         TEXT NOT NULL ,
 OSArch         TEXT NOT NULL ,
 OSDescription  TEXT NOT NULL ,
 OSVersion      TEXT NOT NULL ,
 OSVendor       TEXT NOT NULL ,
 OSVendorName   TEXT NOT NULL ,
 CPUModel       TEXT NOT NULL ,
 CPUVendor      TEXT NOT NULL ,
 CPUFrequencyHz   INT NOT NULL ,
 PhysicalMemoryKB INT NOT NULL ,
 NodeID      INT NOT NULL ,

PRIMARY KEY (SystemID),
CONSTRAINT FK_373 FOREIGN KEY (NodeID) REFERENCES Node (NodeID)
);






-- ************************************** Hardware.CPUStatus

CREATE TABLE Hardware.CPUStatus
(
 CPUStatusID   SERIAL ,
 SequenceNumber  INT NOT NULL ,
 SampleTime      TIMESTAMP NOT NULL ,
 CoreID          INT NOT NULL ,
 CoreUtilisation DECIMAL NOT NULL ,
 NodeID       INT NOT NULL ,

PRIMARY KEY (CPUStatusID),
CONSTRAINT FK_385 FOREIGN KEY (NodeID) REFERENCES Node (NodeID)
);






-- ************************************** Hardware.Process

CREATE TABLE Hardware.Process
(
 ProcessID    SERIAL ,
 pID            SMALLINT NOT NULL ,
 WorkingDirectory TEXT NOT NULL ,
 ProcessName    TEXT NOT NULL ,
 Args           TEXT NOT NULL ,
 StartTime      TIMESTAMP NOT NULL ,
 NodeID      INT NOT NULL ,

PRIMARY KEY (ProcessID),
CONSTRAINT FK_390 FOREIGN KEY (NodeID) REFERENCES Node (NodeID)
);






-- ************************************** Hardware.Interface

CREATE TABLE Hardware.Interface
(
 InterfaceID SERIAL ,
 Name          TEXT NOT NULL ,
 Type          TEXT NOT NULL ,
 Description   TEXT NOT NULL ,
 IPv4          INET NOT NULL ,
 IPv6          INET NOT NULL ,
 MAC           MACADDR NOT NULL ,
 Speed         INT NOT NULL ,
 NodeID     INT NOT NULL ,

PRIMARY KEY (InterfaceID),
CONSTRAINT FK_381 FOREIGN KEY (NodeID) REFERENCES Node (NodeID)
);






-- ************************************** Hardware.Filesystem

CREATE TABLE Hardware.Filesystem
(
 FilesystemID SERIAL ,
 SampleTime     TIMESTAMP NOT NULL ,
 Name           TEXT NOT NULL ,
 Type           TEXT NOT NULL ,
 Size           INT NOT NULL ,
 SequenceNumber INT NOT NULL ,
 NodeID      INT NOT NULL ,

PRIMARY KEY (FilesystemID),
CONSTRAINT FK_377 FOREIGN KEY (NodeID) REFERENCES Node (NodeID)
);






-- ************************************** ComponentInstance

CREATE TABLE ComponentInstance
(
 ComponentInstanceID SERIAL ,
 Path                TEXT NOT NULL ,
 Name                TEXT NOT NULL ,
 GraphmlID           TEXT NOT NULL,
 ContainerID         INT NOT NULL ,
 ComponentID         INT NOT NULL ,

PRIMARY KEY (ComponentInstanceID),
CONSTRAINT UniquePathPerRun UNIQUE (ComponentID, Path),
CONSTRAINT FK_394 FOREIGN KEY (ContainerID) REFERENCES Container (ContainerID),
CONSTRAINT FK_407 FOREIGN KEY (ComponentID) REFERENCES Component (ComponentID)
);






-- ************************************** Hardware.SystemStatus

CREATE TABLE Hardware.SystemStatus
(
 SystemStatusID   SERIAL ,
 SystemID         INT NOT NULL ,
 SampleTime         TIMESTAMP NOT NULL ,
 CPUUtilisation     DECIMAL NOT NULL ,
 PhysMemUtilisation DECIMAL NOT NULL ,

PRIMARY KEY (SystemStatusID),
CONSTRAINT FK_SystemStatus_SystemID_System_SystemID FOREIGN KEY (SystemID) REFERENCES Hardware.System (SystemID)
);






-- ************************************** Hardware.ProcessStatus

CREATE TABLE Hardware.ProcessStatus
(
 ProcessID        INT NOT NULL ,
 ProcessStatusID  SERIAL ,
 CoreID             INT NOT NULL ,
 CPUutilisation     DECIMAL NOT NULL ,
 PhysMemUtilisation DECIMAL NOT NULL ,
 ThreadCount        INTEGER NOT NULL ,
 DiskRead           INTEGER NOT NULL ,
 DiskWritten        INTEGER NOT NULL ,
 DiskTotal          INTEGER NOT NULL ,
 State              TEXT NOT NULL ,
 SampleTime         TIMESTAMP NOT NULL ,

PRIMARY KEY (ProcessStatusID),
CONSTRAINT FK_131 FOREIGN KEY (ProcessID) REFERENCES Hardware.Process (ProcessID)
);






-- ************************************** Hardware.InterfaceStatus

CREATE TABLE Hardware.InterfaceStatus
(
 InterfaceID       INT NOT NULL ,
 InterfaceStatusID SERIAL ,
 PacketsReceived     INT NOT NULL ,
 BytesReceived       INT NOT NULL ,
 PacketsTransmitted  INT NOT NULL ,
 BytesTransmitted    INT NOT NULL ,
 SampleTime          TIMESTAMP NOT NULL ,

PRIMARY KEY (InterfaceStatusID),
CONSTRAINT FK_104 FOREIGN KEY (InterfaceID) REFERENCES Hardware.Interface (InterfaceID)
);






-- ************************************** Hardware.FilesystemStatus

CREATE TABLE Hardware.FilesystemStatus
(
 FilesytemStatusID SERIAL ,
 SampleTime          TIMESTAMP NOT NULL ,
 Utilisation         DECIMAL NOT NULL ,
 FilesystemID      INT NOT NULL ,

PRIMARY KEY (FilesytemStatusID),
CONSTRAINT FK_Filesystem_FileSystem_ID_FileSystem_FilsystemID FOREIGN KEY (FilesystemID) REFERENCES Hardware.Filesystem (FilesystemID)
);






-- ************************************** ClusteredNode

CREATE TABLE ClusteredNode
(
 ClusterID INT NOT NULL ,
 NodeID    INT NOT NULL ,

PRIMARY KEY (ClusterID, NodeID),
CONSTRAINT FK_ClusteredNode_ClusterID_Cluster_Cluster_ID FOREIGN KEY (ClusterID) REFERENCES Cluster (ClusterID),
CONSTRAINT FK_CusteredNode_NodeID_Node_NodeID FOREIGN KEY (NodeID) REFERENCES Node (NodeID)
);






-- ************************************** ComponentLifecycleEvent

CREATE TABLE ComponentLifecycleEvent
(
 ComponentLifecycleEventID SERIAL ,
 ComponentInstanceID       INT NOT NULL ,
 Type                      TEXT NOT NULL ,
 SampleTime                TIMESTAMP NOT NULL ,

PRIMARY KEY (ComponentLifecycleEventID),
CONSTRAINT FK_ComponentLifecycleEvent_ComponentInstanceID_ComponentInstance_ComponentInstanceID FOREIGN KEY (ComponentInstanceID) REFERENCES ComponentInstance (ComponentInstanceID)
);






-- ************************************** UserEvent

CREATE TABLE UserEvent
(
 UserEventID         SERIAL ,
 Message             TEXT NOT NULL ,
 ComponentInstanceID INT NOT NULL ,
 Type                TEXT NOT NULL ,
 SampleTime          TIMESTAMP NOT NULL ,

PRIMARY KEY (UserEventID),
CONSTRAINT FK_UserEvent_ComponentID_Component_ComponentID FOREIGN KEY (ComponentInstanceID) REFERENCES ComponentInstance (ComponentInstanceID)
);






-- ************************************** WorkerInstance

CREATE TABLE WorkerInstance
(
 WorkerInstanceID    SERIAL ,
 Path                TEXT NOT NULL ,
 Name                TEXT NOT NULL ,
 GraphmlID           TEXT NOT NULL,
 ComponentInstanceID INT NOT NULL ,
 WorkerID            INT NOT NULL ,

PRIMARY KEY (WorkerInstanceID),
CONSTRAINT UniqueWorkerNamePerComponent UNIQUE (Name, ComponentInstanceID),
CONSTRAINT FK_Worker_ComponentID_Component_ComponentID FOREIGN KEY (ComponentInstanceID) REFERENCES ComponentInstance (ComponentInstanceID),
CONSTRAINT FK_416 FOREIGN KEY (WorkerID) REFERENCES Worker (WorkerID)
);






-- ************************************** Port

CREATE TABLE Port
(
 PortID              SERIAL ,
 Name                TEXT NOT NULL ,
 Path                TEXT NOT NULL ,
 GraphmlID           TEXT NOT NULL,
 ComponentInstanceID INT NOT NULL ,
 Kind                TEXT NOT NULL ,
 Type                TEXT ,
 Middleware          TEXT ,

PRIMARY KEY (PortID),
CONSTRAINT UniquePortNamePerComponent UNIQUE (ComponentInstanceID, Name),
CONSTRAINT FK_Port_ComponentID_Component_ComponentID FOREIGN KEY (ComponentInstanceID) REFERENCES ComponentInstance (ComponentInstanceID)
);






-- ************************************** PortLifecycleEvent

CREATE TABLE PortLifecycleEvent
(
 PortLifecycleEventID SERIAL ,
 PortID               INT NOT NULL ,
 Type                 TEXT NOT NULL ,
 SampleTime           TIMESTAMP NOT NULL ,

PRIMARY KEY (PortLifecycleEventID),
CONSTRAINT FK_PortLifecycleEvent_PortID_Port_PortID FOREIGN KEY (PortID) REFERENCES Port (PortID)
);






-- ************************************** WorkloadEvent

CREATE TABLE WorkloadEvent
(
 WorkloadEventID  SERIAL ,
 WorkerInstanceID INT NOT NULL ,
 WorkloadID     INT NOT NULL ,
 Function         TEXT NOT NULL ,
 Type             TEXT NOT NULL ,
 Arguments        TEXT NOT NULL ,
 LogLevel         INT NOT NULL ,
 SampleTime       TIMESTAMP NOT NULL ,

PRIMARY KEY (WorkloadEventID),
CONSTRAINT FK_WorkloadEvent_WorkerID_Worker_WorkerID FOREIGN KEY (WorkerInstanceID) REFERENCES WorkerInstance (WorkerInstanceID)
);






-- ************************************** PortEvent

CREATE TABLE PortEvent
(
 PortEventID          SERIAL ,
 PortID               INT NOT NULL ,
 PortEventSequenceNum INT NOT NULL ,
 Type                 TEXT NOT NULL ,
 Message              TEXT ,
 SampleTime           TIMESTAMP NOT NULL ,

PRIMARY KEY (PortEventID),
CONSTRAINT FK_PortEvent_PortID_Port_PortID FOREIGN KEY (PortID) REFERENCES Port (PortID)
);


