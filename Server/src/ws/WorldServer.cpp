//
// Created by FyS on 23/05/17.
//

#include <spdlog/spdlog.h>

#include <boost/lexical_cast.hpp>
#include <WorldServer.hh>
#include <FySAuthenticationLoginMessage.pb.h>
#include <FySMessage.pb.h>

#include <TcpConnection.hh>
#include <WorldEngine.hh>
#include <Map.hh>

/**
 * Timer (in seconds) between re-connection attempt on the Gateway
 */
static constexpr int RETRY_TIMER = 10;
static constexpr char MAGIC_PASSWORD[] = "42Magic42FyS";

fys::ws::WorldServer::~WorldServer() = default;

fys::ws::WorldServer::WorldServer(const fys::ws::Context &ctx, boost::asio::io_service &ios,
                                  std::shared_ptr<fys::mq::FysBus<fys::pb::FySMessage, BUS_QUEUES_SIZE> > fysBus) :
        _ios(ios),
        _acceptorPlayer(_ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), ctx.getPort())),
        _fysBus(std::move(fysBus)),
        _gamerConnections(network::PlayerManager::CONNECTION_NUMBER),
        _worldServerCluster(network::WorldServerCluster::CONNECTION_NUMBER),
        _gtwConnection(std::make_unique<fys::network::TcpConnection>(ios)),
        _worldEngine(std::make_shared<fys::ws::WorldEngine>(ctx.getTmxFileMapName())) {
}

void fys::ws::WorldServer::runPlayerAccept() {
    const network::TcpConnection::ptr session = network::TcpConnection::create(_ios);

    _acceptorPlayer.async_accept(session->getSocket(),

             [this, session](const boost::system::error_code &e) {
                 if (e)
                     spdlog::get("c")->error("An error occurred while connecting a player {}", e.message());
                 else {
                     uint idx = this->_gamerConnections.addPlayerConnection(session);

                     if (idx < std::numeric_limits<uint>::max()) {
                         spdlog::get("c")->info("A player connected with index {}", idx);
                         session->readOnSocket(_fysBus);
                     }
                 }
                 this->runPlayerAccept();
             }

    );
}

void fys::ws::WorldServer::connectToGateway(const fys::ws::Context &ctx) {
    static std::once_flag of = {};
    spdlog::get("c")->info("Connect to the gateway on host: {}, with port: {}", ctx.getGtwIp(), ctx.getGtwPort());
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ctx.getGtwIp()), ctx.getGtwPort());

    std::call_once(of, [this, &ctx](){
        auto disconnectionHandler = [this, &ctx]() {
            boost::asio::deadline_timer timer(_ios);
            spdlog::get("c")->warn("An Error occurred while trying to connect to the gateway, retry in progress");
            timer.expires_from_now(boost::posix_time::seconds(RETRY_TIMER));
            timer.wait();
            this->connectToGateway(ctx);
        };
        _gtwConnection->setCustomShutdownHandler(disconnectionHandler);
    });
    _gtwConnection->getSocket().async_connect(endpoint, [this, &ctx](const boost::system::error_code &error) {
        if (error) {
            _gtwConnection->getCustomShutdownHandler()();
        } else {
            _gtwConnection->readOnSocket(_fysBus);
            this->notifyGateway(ctx.getPositionId(), ctx.getPort());
        }
    });
}

void fys::ws::WorldServer::notifyGateway(const std::string &positionId, const ushort port) const {
    fys::pb::FySMessage msg;
    fys::pb::LoginMessage loginMsg;
    fys::pb::LoginGameServer gameServerMessage;

    gameServerMessage.set_isworldserver(true);
    gameServerMessage.set_portforplayer(std::to_string(port));
    gameServerMessage.set_magicpassword(std::string(MAGIC_PASSWORD).append(positionId));
    loginMsg.set_typemessage(fys::pb::LoginMessage_Type_LoginGameServer);
    loginMsg.mutable_content()->PackFrom(gameServerMessage);
    msg.set_type(fys::pb::AUTH);
    msg.mutable_content()->PackFrom(loginMsg);
    _gtwConnection->send(std::move(msg));

    spdlog::get("c")->debug("Connection with gateway successful, the Gateway has been notified of connection: "
                            "{}", msg.ShortDebugString());
}

void fys::ws::WorldServer::connectAndAddWorldServerInCluster(const std::string &clusterKey, const std::string &token,
                                                             const std::string &ip, const std::string &port) {
    spdlog::get("c")->info("A new WorldServer has been added in cluster: "
                           "positionId {}, ip {}, on port {}", clusterKey, ip, port);


}

void fys::ws::WorldServer::run() {
    std::thread worldLoopThread([this](){
        _worldEngine->runWorldLoop();
    });
    worldLoopThread.detach();
}

void fys::ws::WorldServer::initPlayerPosition(uint indexInSession, float x, float y) {
    _worldEngine->initPlayerMapData(indexInSession, {x, y});
}
