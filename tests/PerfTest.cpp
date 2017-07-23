#include "decode/groundcontrol/GroundControl.h"
#include "decode/groundcontrol/Exchange.h"
#include "decode/groundcontrol/Scheduler.h"
#include "decode/model/ModelEventHandler.h"
#include "decode/model/Model.h"
#include "decode/model/Node.h"
#include "decode/model/Value.h"

#include "photon/Init.h"
#include "photon/exc/Exc.Component.h"
#include "photon/tm/Tm.Component.h"

#include <bmcl/Bytes.h>
#include <bmcl/Logging.h>

#include <tclap/CmdLine.h>

#include <thread>

using namespace decode;

struct ISched : public Scheduler {
public:
    void scheduleAction(StateAction action, std::chrono::milliseconds delay) override
    {
        (void)delay;
        actions.emplace_back(std::move(action));
    }

    std::vector<StateAction> actions;
};

class PhotonSink : public DataSink {
public:
    void sendData(bmcl::Bytes packet) override
    {
        for (uint8_t byte : packet) {
            PhotonExc_AcceptInput(&byte, 1);
        }
    }
};

void walkNode(Node* node)
{
    for (std::size_t i = 0; i < node->numChildren(); i++) {
        Rc<Node> child = node->childAt(i).data();
        if (child) {
            walkNode(child.get());
        }
        auto value = node->value();
    }
}

class EventHandler : public ModelEventHandler {
public:
    void beginHashDownload() override
    {
        BMCL_DEBUG() << "fwt begin";
    }
/*
    void endHashDownload(const std::string& deviceName, bmcl::Bytes firmwareHash);
    void beginFirmwareStartCommand();
    void endFirmwareStartCommand();
    void beginFirmwareDownload(std::size_t firmwareSize);
    void firmwareError(const std::string& errorMsg);*/
    void firmwareDownloadProgress(std::size_t sizeDownloaded) override
    {
        BMCL_DEBUG() << sizeDownloaded;
    }

    void endFirmwareDownload() override
    {
        BMCL_DEBUG() << "fwt ok";
    }
//     void packetQueued(bmcl::Bytes packet);
//     void modelUpdated(const Rc<Model>& model) override;



//     void nodeValueUpdated(const Node* node, std::size_t nodeIndex);
//     void nodesInserted(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
//     void nodesRemoved(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
};

uint8_t temp[2048];

int main(int argc, char** argv)
{
    TCLAP::CmdLine cmdLine("PhotonUi");
    TCLAP::ValueArg<std::size_t> iArg("i", "num", "Iterations", false, 10000, "num iterations");

    cmdLine.add(&iArg);
    cmdLine.parse(argc, argv);

    PhotonTm_Init();
    Photon_Init();
    PhotonExcMsg current = *PhotonExc_GetMsg();

    Rc<ISched> sched = new ISched;
    Rc<EventHandler> handler = new EventHandler;
    Rc<PhotonSink> sink = new PhotonSink;
    Rc<GroundControl> gc = new GroundControl(sink.get(), sched.get(), handler.get());
    gc->start();

    auto start = std::chrono::steady_clock::now();
    std::thread exchangeThread([&]() {
        for (std::size_t i = 0; i < iArg.getValue(); i++) {
            gc->acceptData(bmcl::Bytes(current.data, current.size));
            PhotonExc_PrepareNextMsg();
            current = *PhotonExc_GetMsg();

            std::vector<StateAction> actions = std::move(sched->actions);
            for (const StateAction& action : actions) {
                action();
            }
        }
    });
    exchangeThread.join();

    sched->actions.clear();

    auto end = std::chrono::steady_clock::now();
    auto delta = end - start;
    BMCL_DEBUG() << "time (us):" << std::chrono::duration_cast<std::chrono::microseconds>(delta).count();
}
