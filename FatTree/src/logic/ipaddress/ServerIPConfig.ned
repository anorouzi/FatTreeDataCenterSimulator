//
// Author: Michael Klopf
// 

package fattree.logic.ipaddress;

//
// Module assigns ip addresses to the servers of a fat tree network.
//
simple ServerIPConfig
{
    string networkAddress = default("10.0.0.0"); // network part of the address (see netmask parameter)
    string netmask = default("255.0.0.0"); // host part of addresses are autoconfigured
    @display("i=block/cogwheel_s");
    @labels(node);
}
