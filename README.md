# Subnet Subtractor
A tool for subtracting sets of subnets from each other

### What is this for?
Let's say you want to create route or filewall rule that matches a certain subnet except an even smaller subnet within that range. What whould be the resulting set of subnets that matches only exactly those IPs? Yes, you can easily calculate this manually - or just use a tiny little tool, that does exactly this for you. Here you go.
This little tool automatically splits, joins, and subtracts one set of subnets from another sets of subnets and returns the smallest set of subnets that matches exactly the given specs using the shortest network prefixes.

### Can it do IPv6 as well?
Actually it can only do IPv6 underneath the hood, yet it even nicely handles IPv4 for convenience.

### What's the state of this project?
This is just a tiny little quick hack with a remarkably awful Makefile, file- and code-structure. But good luck anyway.

### How to compile and run?
Use `make`.

Here's an example for running this tool and its corresponding output:
```
[user@host ~]$ ./subnet-subtractor 192.168.0.0/17 +192.168.128.0/17 -192.168.178.0/24 -192.168.179.0/24
Given Included Subnets:
+ 192.168.0.0/16

Given Excluded Subnets:
- 192.168.178.0/23

Included Subnets without Excluded Subnets:
= 192.168.0.0/17
= 192.168.192.0/18
= 192.168.128.0/19
= 192.168.160.0/20
= 192.168.184.0/21
= 192.168.180.0/22
= 192.168.176.0/23
```

### Why would I want to compile a tool for that?
You don't need to. Just do it on the web: [Visual Subnet Calculator](https://www.davidc.net/sites/default/subnets/subnets.html)

![-](https://miunske.eu/github/?subnetcalc)
