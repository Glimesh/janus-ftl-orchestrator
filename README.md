# Janus FTL Orchestrator

This is a work-in-progress service meant to manage multiple instances of [Janus](https://github.com/meetecho/janus-gateway) running the [Janus FTL Plugin](https://github.com/Glimesh/janus-ftl-plugin).

By orchestrating many Janus instances with this service, load from ingest and viewers can be distributed and greater scale can be achieved.

# Building

## Dependencies

`libspdlog-dev`