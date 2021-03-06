//
// Author: Michael Klopf
// 

#include "ConfigRouting.h"
#include <IRoutingTable.h>
#include <IInterfaceTable.h>
#include <InterfaceTable.h>
#include <InterfaceEntry.h>
#include <IPvXAddress.h>
#include <IPAddressResolver.h>

Define_Module(ConfigRouting);

void ConfigRouting::initialize(int stage)
{
    if (stage == 2) {
        // Extract topology of network
        cTopology topo("fattree");
        topo.extractByProperty("node");
        EV << "cTopology found " << topo.getNumNodes() << " nodes\n";

        // Create node infos for default routes knowledge.
        NodeInfoVector nodeInfo;
        nodeInfo.resize(topo.getNumNodes());

        // add default routes to hosts (nodes with a single attachment);
        // also remember result in nodeInfo[].usesDefaultRoute
        addDefaultRoutes(topo, nodeInfo);

        // calculate shortest paths, and add corresponding static routes
        fillRoutingTables(topo, nodeInfo);
    }
}

void ConfigRouting::addDefaultRoutes(cTopology& topo, NodeInfoVector& nodeInfo) {
    // add default route to nodes with exactly one (non-loopback) interface
    for (int i = 0; i < topo.getNumNodes(); i++) {
        cTopology::Node *node = topo.getNode(i);
        cModule *mod = node->getModule();

        // skip bus types
        if (IPAddressResolver().findInterfaceTableOf(mod)==NULL) {
            continue;
        }
        IInterfaceTable *ift = IPAddressResolver().findInterfaceTableOf(mod);
        IRoutingTable *rt = IPAddressResolver().routingTableOf(mod);

        // count non-loopback interfaces
        int numIntf = 0;
        InterfaceEntry *ie = NULL;
        for (int k = 0; k < ift->getNumInterfaces(); k++) {
            if (!ift->getInterface(k)->isLoopback()) {
                ie = ift->getInterface(k);
                numIntf++;
            }
        }
        // Set information about the node using the default route or not.
        nodeInfo[i].usesDefaultRoute = (numIntf == 1);

        // When rerouting routine, then skip adding routes, they already exist.
        if (isReRouting) {
        	continue;
        }

        // When default route not set then continue.
        if (numIntf!=1)
            continue;

        // add route
        IPRoute *e = new IPRoute();
        e->setHost(IPAddress());
        e->setNetmask(IPAddress());
        e->setInterface(ie);
        e->setType(IPRoute::REMOTE);
        e->setSource(IPRoute::MANUAL);
        rt->addRoute(e);
    }
}

void ConfigRouting::fillRoutingTables(cTopology& topo, NodeInfoVector& nodeInfo) {
    // fill in routing tables with static routes
    for (int i = 0; i < topo.getNumNodes(); i++) {
        cTopology::Node *destNode = topo.getNode(i);
        cModule *destMod = destNode->getModule();

        // skip bus types
        if (IPAddressResolver().findInterfaceTableOf(destMod)==NULL) {
            continue;
        }

        IInterfaceTable *ift = IPAddressResolver().interfaceTableOf(destMod);

        int addrType = IPAddressResolver::ADDR_IPv4;
        IPvXAddress destAddr = IPAddressResolver().getAddressFrom(ift,addrType);

        std::string destModName = destMod->getFullName();

        // calculate shortest paths from everywhere towards destNode
        topo.calculateUnweightedSingleShortestPathsTo(destNode);

        for (int j = 0; j < topo.getNumNodes(); j++) {
            if (i==j) continue; // destination is source
            if (IPAddressResolver().findInterfaceTableOf(destMod)==NULL) {
                continue; // check if bus type
            }

            cTopology::Node *atNode = topo.getNode(j);
            cModule *atMod = atNode->getModule();

            if (atNode->getNumPaths()==0)
                continue; // not connected
            if (nodeInfo[j].usesDefaultRoute)
                continue; // already added default route here

            IInterfaceTable *atift = IPAddressResolver().interfaceTableOf(atMod);
            IPvXAddress atAddr = IPAddressResolver().getAddressFrom(atift,addrType);

            int outputGateId = atNode->getPath(0)->getLocalGate()->getId();
            InterfaceEntry *ie = atift->getInterfaceByNodeOutputGateId(outputGateId);

            if (!ie)
            error("%s has no interface for output gate id %d", atift->getFullPath().c_str(), outputGateId);

            //EV << "  from " << atNode->getModule()->getFullName() << "=" << IPAddress(atAddr.get4());
            //EV << " towards " << destModName << "=" << IPAddress(destAddr.get4()) << " interface " << ie->getName() << endl;

            // add route
            IRoutingTable *rt = IPAddressResolver().routingTableOf(atMod);
            IPRoute *e = new IPRoute();
            e->setHost(destAddr.get4());
            e->setNetmask(IPAddress(255,255,255,255)); // full match needed
            e->setInterface(ie);
            e->setType(IPRoute::DIRECT);
            e->setSource(IPRoute::MANUAL);
            rt->addRoute(e);
        }
    }
}

void ConfigRouting::fillRoutingTables(cTopology& topo, NodeInfoVector& nodeInfo, const char * vm1address, const char * vm2address) {
    // fill in routing tables with static routes
    for (int i = 0; i < topo.getNumNodes(); i++) {
        cTopology::Node *destNode = topo.getNode(i);
        cModule *destMod = destNode->getModule();

        // skip bus types
        if (IPAddressResolver().findInterfaceTableOf(destMod)==NULL) {
            continue;
        }

        IInterfaceTable *ift = IPAddressResolver().interfaceTableOf(destMod);

        int addrType = IPAddressResolver::ADDR_IPv4;
        IPvXAddress destAddr = IPAddressResolver().getAddressFrom(ift,addrType);

        if (destAddr == IPvXAddress(vm1address) || destAddr == IPvXAddress(vm2address)) {
            std::string destModName = destMod->getFullName();

            // calculate shortest paths from everywhere towards destNode
            topo.calculateUnweightedSingleShortestPathsTo(destNode);

            for (int j = 0; j < topo.getNumNodes(); j++) {
                if (i==j) continue; // destination is source
                if (IPAddressResolver().findInterfaceTableOf(destMod)==NULL) {
                    continue; // check if bus type
                }

                cTopology::Node *atNode = topo.getNode(j);
                cModule *atMod = atNode->getModule();

                if (atNode->getNumPaths()==0)
                    continue; // not connected
                if (nodeInfo[j].usesDefaultRoute)
                    continue; // already added default route here

                IInterfaceTable *atift = IPAddressResolver().interfaceTableOf(atMod);
                IPvXAddress atAddr = IPAddressResolver().getAddressFrom(atift,addrType);

                int outputGateId = atNode->getPath(0)->getLocalGate()->getId();
                InterfaceEntry *ie = atift->getInterfaceByNodeOutputGateId(outputGateId);

                if (!ie)
                error("%s has no interface for output gate id %d", atift->getFullPath().c_str(), outputGateId);

                //EV << "  from " << atNode->getModule()->getFullName() << "=" << IPAddress(atAddr.get4());
                //EV << " towards " << destModName << "=" << IPAddress(destAddr.get4()) << " interface " << ie->getName() << endl;

                // add route
                IRoutingTable *rt = IPAddressResolver().routingTableOf(atMod);

                // delete old routes to vm1/vm2
                IPRoute const *oldroute = rt->findRoute(destAddr.get4(),IPAddress("255.255.255.255"),IPAddress());
                if (!oldroute) {
                	opp_error("Didn't find old route in routing table!");
                }
                rt->deleteRoute(oldroute);

                IPRoute *e = new IPRoute();
                e->setHost(destAddr.get4());
                e->setNetmask(IPAddress(255,255,255,255)); // full match needed
                e->setInterface(ie);
                e->setType(IPRoute::DIRECT);
                e->setSource(IPRoute::MANUAL);
                rt->addRoute(e);
            } // end for-loop
        } // end if for destaddr
    }
}

void ConfigRouting::handleMessage(cMessage *msg)
{
    error("this module doesn't handle messages, it runs only in initialize()");
}

void ConfigRouting::refreshRoutingTables(const char * vm1address, const char * vm2address)
{
	this->isReRouting = true;
    // Extract topology of network
    cTopology topo("fattree");
    topo.extractByProperty("node");

    // Create node infos for default routes knowledge.
    NodeInfoVector nodeInfo;
    nodeInfo.resize(topo.getNumNodes());

    // add default routes to hosts (nodes with a single attachment);
    // also remember result in nodeInfo[].usesDefaultRoute
    addDefaultRoutes(topo, nodeInfo);

    // calculate shortest paths, and add corresponding static routes
    fillRoutingTables(topo, nodeInfo, vm1address, vm2address);
}
