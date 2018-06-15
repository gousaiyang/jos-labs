# Lab 6: Network Driver

## Exercise & Question Answers

Q1: How did you structure your transmit implementation? In particular, what do you do if the transmit ring is full?

A1: If the transmit ring is full, simply report this to user by returning `-E_TX_FULL`. User programs (e.g. the output environment) retry if they wish.

Q2: How did you structure your receive implementation? In particular, what do you do if the receive queue is empty and a user environment requests the next incoming packet?

A2: If the receive queue is empty, simply report this to user by returning `-E_RX_EMPTY`. User programs (e.g. the input environment) retry if they wish.

Q3: What does the web page served by JOS's web server say?

A3: "This file came from JOS. Cheesy web page!"

Q4: How long approximately did it take you to do this lab?

A4: About 20 hours.

## Challenge

I choose to implement the challenge to load the E1000's MAC address out of the EEPROM.

- We load E1000's MAC address through MMIO and modify the initialization function `e1000_attach()` to use the dynamically loaded MAC address instead of the hard-coded one.
- Then we add a system call `sys_net_get_mac()` to allow user programs to retrieve the MAC address.
- Finally we modify `net/lwip/jos/jif/jif.c` to call `sys_net_get_mac()` to dynamically load the MAC address.
