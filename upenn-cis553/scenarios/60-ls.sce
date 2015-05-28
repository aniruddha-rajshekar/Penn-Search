* DV VERBOSE ALL OFF
* LS VERBOSE ALL OFF
* APP VERBOSE ALL OFF
* DV VERBOSE STATUS ON
* DV VERBOSE ERROR ON
* LS VERBOSE STATUS ON
* LS VERBOSE ERROR ON
* APP VERBOSE STATUS ON
* APP VERBOSE ERROR ON
* LS VERBOSE TRAFFIC ON
* APP VERBOSE TRAFFIC ON

# Allow 120s for initial convergence
TIME 120000

# Test neighbor tables
22 LS DUMP NEIGHBORS

12 LS DUMP NEIGHBORS

# Test routing tables
0 LS DUMP ROUTES

# Test ping
0 APP PING 54 HELLO

# Wait for traffic trace 
TIME 60000

# Bring down a link
LINK DOWN 3 18

TIME 60000

# Test routing table after link failure
0 LS DUMP ROUTES

# Test ping after link failure
3 APP PING 18 HELLO

#Wait for traffic trace
TIME 60000

# Isolate node 0
LINK DOWN 0 1

LINK DOWN 0 22

TIME 60000

# Test ping
0 APP PING 3 HELLO

# Wait for traffic trace
TIME 60000

# End of test
# QUIT



