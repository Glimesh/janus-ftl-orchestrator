# FTL Orchestration Protocol

In order to provide a video streaming service that can handle many viewers and many streamers at once, video must be distributed between many server instances to accommodate the bandwidth demand.

![Video Routing Diagram](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Glimesh/janus-ftl-orchestrator/main/docs/uml/stream-routing-use-case.plantuml)

_[Video Routing Diagram](uml/stream-routing-use-case.plantuml): This diagram visualizes the high-level flow of video traffic from streamers through relay servers to edge nodes that viewers connect to._

To most efficiently coordinate routing of traffic between these many servers, a separate service is being developed called the FTL Orchestrator.

This document outlines the simple protocol used by [janus-ftl-orchestrator](https://github.com/Glimesh/janus-ftl-orchestrator) and [janus-ftl-plugin](https://github.com/Glimesh/janus-ftl-plugin) to communicate with each other and exchange information used to distribute video load across multiple Janus instances.

# Goals and Non-Goals

## Goals

- Routes can be established allowing Janus Edge nodes to serve stream data that is relayed from other Ingest or Relay nodes.
- Ingest nodes are notified when they need to relay streams to other nodes.
- By subscribing to channels, Edge nodes can receive stream relays.
- Stream data can be optionally routed through Relay nodes before reaching Edge nodes.
- Nodes can indicate their approximate geographic region, allowing the Ochestrator to prioritize proximally efficient routes.
- Nodes can indicate their current load, allowing the Orchestrator to avoid creating route bottlenecks.
- Nodes are not sent video stream data unless there are viewers.
- System is resilient to Janus instances dynamically coming online and going offline.

## Non-Goals

- Automatic provisioning and/or destroying of Janus instances
- Handling of stream data (audio, video, etc.)

# Routing Strategy

![Routing Strategy Activity Diagram](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Glimesh/janus-ftl-orchestrator/main/docs/uml/routing-strategy-activity.plantuml)

_[Routing Strategy Activity Diagram](uml/routing-strategy-activity.plantuml): This diagram visualizes the routing strategy that the Orchestrator will use to route streams from ingest nodes to edge nodes._

## Routing Considerations

Nodes will provide a region code when connecting to the Orchestration service. This region code will be used to prioritize routes between nodes in the same region.

Nodes can indicate their current and maximum load via the `Node State` message. This will be used to distribute routes between nodes to minimize load.

# Protocol (v0.0.1)

The FTL Orchestration protocol is a simple binary messaging format transmitted over a TLS-secured TCP socket.

## Authentication + Security

Authentication and encryption are handled by the use of TLS to establish an encrypted transport layer on top of a standard TCP socket. Pre-shared keys are be utilized to establish a TLS connection between clients and the FTL orchestration service.

Any client that successfully completes the TLS handshake is considered fully authorized across the entire surface area of the FTL Orchestration API.

## Message Format

The FTL Orchestration Protocol will operate via a simple binary messaging format. The format of a message consists of a simple header followed by a payload.

### Header Format

```
|-                       32 bit / 4 byte                       -|
+---------------------------------------------------------------+
|  Msg Desc (8)  |   Msg Id (8)   |     Payload Length (16)     |
+---------------------------------------------------------------+
```

| Field            | Size    | Description |
| ---------------  | ------  | ----------- |
| `Msg Desc`       | 8 bits  | Bit field describing the type of message |
| `Msg Id`         | 8 bits  | Unsigned integer value specifying the ID of the message, used to correlate response messages. |
| `Payload Length` | 16 bits | Unsigned integer value specifying the length in bytes of the payload data that follows the header. |

### Msg Desc

`Msg Desc` is a bit field of length 8 that identifies the type of message being sent (and the format of the payload that follows, if present).

```
|-   msg desc    -|
  0 0 0 0 0 0 0 0
  | | |         |
  | | |---------|- type (unsigned, 6 bits)
  | |
  | |- success/failure bit
  |
  |- request/response bit
```

The `request/response` bit allows the client to easily determine whether the message being received is in response to an earlier request, or a new request. `0`: Request, `1`: Response.

The `success/failure` bit allows the client to easily determine whether this message is indicating a success state or a failure state. `0`: Success, `1`: Failure.

This leaves 6 bits for the `type` field to indicate the type of the message.

| Type (dec)   | Name                    | Description |
| ------------ | ----------------------- | ----------- |
| `0`          | Intro                   | Sent on connect with identifying information. |
| `1`          | Outro                   | Sent on disconnect with information on the reason for disconnect. |
| `2`          | Node State              | Sent periodically by nodes to indicate their current state. |
| _`3` - `15`_ | _Reserved_              | _Reserved for future use (server state messaging)_ |
| `16`         | Channel Subscription    | Indicates whether streams for a given channel should be relayed to this node. |
| `17`         | Stream Publishing       | Indicates that a new stream is now available (or unavailable) from this connection. |
| `20`         | Stream Relaying         | Contains information used for relaying streams between nodes. |
| `19` - `31`  | _Reserved_              | _Reserved for future use_ |
| `32` - `63`  | _Reserved_              | _Reserved for future use_ |

### Msg Id

`Msg Id` is a field used to correlate requests with responses. A client can send a message with a unique `Msg Id` and monitor the `Msg Id` of incoming messages to discern the message that is in response to that request.

### Message Payloads

The table below describes the payload format of each message type.

| Message Type                | Request Payload | Response Payload |
| --------------------------- | --------------- | ---------------- |
| `0` / Intro                 | 8-bit unsigned int protocol version major<br />8-bit unsigned int protocol version minor<br />8-bit unsigned integer protocol version revision<br />8-bit unsigned integer relay layer (`0` = not a relay)<br />16-bit unsigned integer region code length<br />ASCII region code<br />ASCII string hostname of node | None |
| `1` / Outro                 | ASCII string describing reason for disconnect | None |
| `2` / Node State            | 32-bit unsigned int current load units<br />32-bit unsigned int maximum load units | None |
| `16` / Channel Subscription | 8-bit context value: `1` = subscribe, `0` = unsubscribe<br />32-bit unsigned integer channel ID<br />If subscribing, binary stream key for relayed streams to use | None |
| `17` / Stream Publishing    | 8-bit context value: `1` = publish, `0` = unpublish<br />32-bit unsigned integer channel ID<br />32-bit unsigned integer stream ID | None |
| `20` / Stream Relaying      | 8-bit context value: `1` = relay stream, `0` = stop relaying stream<br />32-bit unsigned integer channel ID<br />32-bit unsigned stream ID<br />16-bit unsigned integer target hostname length<br />ASCII target hostname string<br />Binary stream key | None |

# Usage Examples

## Typical use case for FTL Ingest

1. Establish connection to FTL orchestration service, send **Intro** request message
2. When an ingest begins, send a **Stream Available** request with the details of the new stream
3. When a **Stream Start Relay** message is received, connect to the indicated node and relay the given stream via the FTL protocol with the provided key.
4. When a **Stream End Relay** message is received, shut down the FTL relay connection for the given stream.
3. When an ingest ends, send a **Stream Removed** request with the details of the ended stream
4. When the service is shutting down, send an **Outro** request message

## Typical use case for FTL Edge/WebRTC

1. Establish connection to FTL orchestration service, send **Intro** request message
2. When a viewer requests to watch a channel, send a **Subscribe Channel** request message for that channel, include a generated stream key that will be used for the FTL relay connection.
3. Listen for incoming stream relays via the FTL protocol.
4. If no more viewers are present for a previously subscribed channel, send a **Unsubscribe Channel** request message for that channel
5. When the service is shutting down, send an **Outro** request message

## UML Sequence Diagram

![FTL Orchestration Sequence Diagram, No Relays](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Glimesh/janus-ftl-orchestrator/main/docs/uml/sequence-diagram-no-relays.plantuml)

_[FTL Orchestration Sequence Diagram, No Relays](uml/sequence-diagram-no-relays.plantuml): This diagram shows the expected sequence of calls to route a stream in a service graph without any relay nodes._

![FTL Orchestration Sequence Diagram, Relays](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Glimesh/janus-ftl-orchestrator/main/docs/uml/sequence-diagram-with-relays.plantuml)

_[FTL Orchestration Sequence Diagram, Relays](uml/sequence-diagram-with-relays.plantuml): This diagram shows the expected sequence of calls to route a stream in a service graph with relay nodes between ingests and edges._
