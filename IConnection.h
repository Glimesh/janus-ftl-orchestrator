/**
 * @file IConnection.h
 * @author Hayden McAfee (hayden@outlook.com)
 * @date 2020-10-25
 * @copyright Copyright (c) 2020 Hayden McAfee
 */

#pragma once

#include <functional>
#include <string>

/**
 * @brief Describes a type of FTL server instance
 */
enum class FtlServerKind
{
    Unknown = 0, // Unknown/uninitialized value
    Ingest = 1,  // FTL Ingest server, used to receive video from streamers and relay it
    Node = 2,    // CURRENTLY UNUSED. FTL Node server, used as an additional relay point b/t ingests and edges
    Edge = 3,    // FTL Edge server, used to relay stream data from ingest servers to viewers.
};

/**
 * @brief IConnection represents a generic connection to an FTL instance.
 */
class IConnection
{
public:
    virtual ~IConnection() = default;

    /**
     * @brief Translates FtlServerKind enum to a human readable string value
     * 
     * @param serverKind server kind enum value
     * @return std::string readable string value
     */
    static std::string FtlServerKindToString(FtlServerKind serverKind)
    {
        switch (serverKind)
        {
        case FtlServerKind::Edge:
            return "Edge";
        case FtlServerKind::Node:
            return "Node";
        case FtlServerKind::Ingest:
            return "Ingest";
        case FtlServerKind::Unknown:
        default:
            return "Unknown";
        }
    }

    /**
     * @brief Starts the connection in a new thread
     */
    virtual void Start() = 0;

    /**
     * @brief Shuts down connection to client and terminates connection thread
     */
    virtual void Stop() = 0;

    /**
     * @brief
     *  Sets the callback that will fire when this connection has been closed.
     *  Note that these events may come in on a new thread, so take care to synchronize
     *  any updates that occur as a result.
     * 
     * @param onConnectionClosed callback to fire on connection close
     */
    virtual void SetOnConnectionClosed(std::function<void(void)> onConnectionClosed) = 0;

    /**
     * @brief Retrieve the hostname of the FTL server represented by this connection
     * 
     * @return std::string the hostname
     */
    virtual std::string GetHostname() = 0;

    /**
     * @brief Get the Server Kind of the FTL server represented by this connection
     * 
     * @return FtlServerKind kind of FTL server
     */
    virtual FtlServerKind GetServerKind() = 0;
};