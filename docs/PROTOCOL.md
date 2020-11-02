# FTL Orchestration Protocol

This document outlines the simple protocol used by [janus-ftl-orchestrator](https://github.com/Glimesh/janus-ftl-orchestrator) and [janus-ftl-plugin](https://github.com/Glimesh/janus-ftl-plugin) to communicate with each other and exchange information used to distribute video load across multiple Janus instances.

## Goals

- An arbitrary Janus instance serving WebRTC content to browser viewers can locate a source of stream data for a given channel/stream.
- An arbitrary Janus instance serving WebRTC content to browser viewers can report viewership information to the origin of the stream.
- A Janus ingest instance can alert other Janus viewer instances that a new stream has started.
- System is resilient to Janus instances dynamically coming online and going offline.

## Non-Goals

- Automatic provisioning and/or destroying of Janus instances
- Handling of video data

# Protocol

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

| Type (dec)   | Name                | Description |
| ------------ | ------------------- | ----------- |
| `0`          | Intro               | Sent on connect with identifying information. |
| `1`          | Outro               | Sent on disconnect with information on the reason for disconnect. |
| _`2` - `15`_ | _Reserved_          | _Reserved for future use (server state messaging)_ |
| `16`         | Subscribe Channel   | Request updates on a given channel. |
| `17`         | Unsubscribe Channel | Request for no further updates on a given channel. |
| `18` - `19`  | _Reserved_          | _Reserved for future use (stream subscription management)_ |
| `20`         | Stream Available    | Indicates that a new stream is now present. |
| `21`         | Stream Removed      | Indicates that an existing stream is no longer present. |
| `22`         | Stream Metadata     | Contains an update to stream metadata (such as viewership) |
| `23`         | _Reserved_          | _Reserved for future use (stream updates)_ |
| `24` - `63`  | _Reserved_          | _Reserved for future use_ |

### Msg Id

`Msg Id` is a field used to correlate requests with responses. A client can send a message with a unique `Msg Id` and monitor the `Msg Id` of incoming messages to discern the message that is in response to that request.

### Message Payloads

The table below describes the payload format of each message type.

| Message Type                | Payload |
| --------------------------- | ------- |
| `0` / Intro                 | 8-bit unsigned int major version<br />8-bit unsigned int minor version<br />8-bit unsigned integer revision<br />ASCII string hostname of client |
| `1` / Outro                 | ASCII string describing reason for disconnect |
| `16` / Subscribe Channel    | 32-bit unsigned integer channel ID |
| `17` / Unsubscribe Channel  | 32-bit unsigned integer channel ID |
| `20` / Stream Available     | 32-bit unsigned integer channel ID<br />32-bit unsigned integer stream ID<br />ASCII hostname string for Ingest |
| `21` / Stream Removed       | 32-bit unsigned integer channel ID<br />32-bit unsigned integer stream ID |
| `22` / Stream Metadata      | 32-bit unsigned integer channel ID<br />32-bit unsigned integer stream ID<br />32-bit unsigned integer viewer count |

# Usage Examples

## Typical use case for FTL Ingest

1. Establish connection to FTL orchestration service, send **Intro** request message
2. When an ingest begins, send a **Stream Available** request with the details of the new stream
3. When an ingest ends, send a **Stream Removed** request with the details of the ended stream
4. When the service is shutting down, send an **Outro** request message

## Typical use case for FTL Edge/WebRTC

1. Establish connection to FTL orchestration service, send **Intro** request message
2. When a viewer requests to watch a channel, send a **Subscribe Channel** request message for that channel
3. When a **Stream Available** request is received, extract the hostname for the stream's ingest from the message payload, then connect to the specified ingest server and begin relaying video (if viewers are present for the channel indicated)
4. When a **Stream Removed** request is received, stop relaying video to viewers, and indicate to them that the stream has ended
5. If no more viewers are present for a previously subscribed channel, send a **Unsubscribe Channel** request message for that channel
6. When the service is shutting down, send an **Outro** request message

## UML Sequence Diagram

![Typical FTL Orchestration Service Flow](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/Glimesh/janus-ftl-orchestrator/protocol-doc/docs/uml/typical-flow.plantuml)

# Open Questions

- What is the protocol for contacting ingest servers to begin relaying video?