//
// Author: Michael Klopf
// 

#ifndef __FATTREEVMMIGRATIONCASE1_EDGEIPCONFIG_H_
#define __FATTREEVMMIGRATIONCASE1_EDGEIPCONFIG_H_

#include <omnetpp.h>
#include <IPAddress.h>

/**
 * Sets the IP address of the edge router respectively rack/ToR at a certain position
 * in dependence to the k-value of the fat tree.
 */
class EdgeIPConfig : public cSimpleModule
{
    protected:
        /**
         * gives the position among the other core routers.
         */
        int position;
        /**
         * gives the position of the POD the router lies in.
         */
        int podposition;
  protected:
    virtual int numInitStages() const  {return 2;}
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);
    /**
     * Takes the k-value, the position of the aggregation router in the
     * pod and the position of the pod among the pods to compute the
     * ip address.
     */
    virtual IPAddress createAddress(int position, int podposition);
    /**
     * Finds the interface table and assigns the ip address
     * to all non-loopback interfaces.
     */
    virtual void setRouterIPAddress(IPAddress address);
    /**
     * Sets the ip address as display string in graphical environment.
     */
    virtual void setDisplayString(IPAddress address);
};

#endif
