//
// Created by FyS on 18/03/18.
//

#include <spdlog/spdlog.h>

#include <google/protobuf/util/time_util.h>
#include <FySPlayerInteraction.pb.h>
#include <FySMessage.pb.h>

#include <WorldServer.hh>
#include <WorldEngine.hh>
#include "GamingListener.hh"

using namespace google::protobuf::util;

namespace fys::ws {

buslistener::GamingListener::GamingListener(WorldServer::ptr &ws) :
        _ws(ws), _worldEngine(ws->getWorldEngine()){
}

void buslistener::GamingListener::operator()(fys::mq::QueueContainer<pb::FySMessage> msg) {
    pb::PlayerInteract playerInteract;

    msg.getContained().content().UnpackTo(&playerInteract);
    if (_ws->getGamerConnections().isAuthenticated(msg.getIndexSession(), playerInteract.token())) {
        spdlog::get("c")->error("Bad authentication used in GamingListener user {} with token {}",
                                msg.getIndexSession(), playerInteract.token());
    }
    else if (pb::PlayerInteract_Type_IsValid(playerInteract.type())) {
        switch (playerInteract.type()) {

            case pb::PlayerInteract::MOVE_ON:
                changePlayerStateInteractionMove(msg.getIndexSession(), std::move(playerInteract));
                break;

            case pb::PlayerInteract::MOVE_OFF:
                changePlayerStatInteractionStopMoving(msg.getIndexSession());
                break;

            case pb::PlayerInteract::ACTIVATE:
                playerInteractionWithWorldItem(msg.getIndexSession(), std::move(playerInteract));
                break;

            case pb::PlayerInteract::REQUEST_INFO:
                playerRequestInformation(msg.getIndexSession(), std::move(playerInteract));
                break;

            default:
                break;
        }
    }
}

void buslistener::GamingListener::changePlayerStateInteractionMove(uint idx, pb::PlayerInteract &&interact) {
    fys::pb::PlayerMove playerMove;
    interact.content().UnpackTo(&playerMove);
    _worldEngine->changePlayerMovingState(idx, TimeUtil::TimestampToMilliseconds(interact.time()), playerMove.angle());
    _worldEngine->notifyNeighbourhoodServer(idx, _ws->getWorldServerCluster());
}

void buslistener::GamingListener::changePlayerStatInteractionStopMoving(uint idx) {
    _worldEngine->changePlayerMovingState(idx);
}

void
buslistener::GamingListener::playerRequestInformation(uint idx, pb::PlayerInteract &&interact) {

}

void buslistener::GamingListener::playerInteractionWithWorldItem(uint idx, pb::PlayerInteract &&interact) {

}

}