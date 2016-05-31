# event_loop
event_loop based on hostpad eventloop
## eloop usage
- include eloop.h
-- and include eloop_callback_if.h on C++
-- register C++ method instead of C function callback
- register callback
- use async event driven programming on linux/cygwin/visualC++
- use green thread(task) on sequential programming on eloop
- these eloop can be use bare-metal environment with few function porting.
- exception handling on C including SEGV exception.
