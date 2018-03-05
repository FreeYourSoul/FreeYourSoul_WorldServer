#include <spdlog/spdlog.h>
#include <iostream>
#include <thread>
#include <WorldServer.hh>
#include <BusListener.hh>
#include <Authenticator.hh>
#include <TokenGenerator.hh>

static const std::string welcomeMsg =
        "\n"
        "                                                                                                           \n"
        "                                                                                                           \n"
        "____              ___               ___      ___         ____                                              \n"
        "`Mb(      db      )d'               `MM      `MM        6MMMMb\\                                            \n"
        " YM.     ,PM.     ,P                 MM       MM       6M'    `                                            \n"
        " `Mb     d'Mb     d'  _____  ___  __ MM   ____MM       MM         ____  ___  __ ____    ___  ____  ___  __ \n"
        "  YM.   ,P YM.   ,P  6MMMMMb `MM 6MM MM  6MMMMMM       YM.       6MMMMb `MM 6MM `MM(    )M' 6MMMMb `MM 6MM \n"
        "  `Mb   d' `Mb   d' 6M'   `Mb MM69 \" MM 6M'  `MM        YMMMMb  6M'  `Mb MM69 \"  `Mb    d' 6M'  `Mb MM69 \" \n"
        "   YM. ,P   YM. ,P  MM     MM MM'    MM MM    MM            `Mb MM    MM MM'      YM.  ,P  MM    MM MM'    \n"
        "   `Mb d'   `Mb d'  MM     MM MM     MM MM    MM             MM MMMMMMMM MM        MM  M   MMMMMMMM MM     \n"
        "    YM,P     YM,P   MM     MM MM     MM MM    MM             MM MM       MM        `Mbd'   MM       MM     \n"
        "    `MM'     `MM'   YM.   ,M9 MM     MM YM.  ,MM       L    ,M9 YM    d9 MM         YMP    YM    d9 MM     \n"
        "     YP       YP     YMMMMM9 _MM_   _MM_ YMMMMMM_      MYMMMM9   YMMMM9 _MM_         M      YMMMM9 _MM_    \n"
        "                                                                                                           \n"
        "                                                                                                           \n"
        "                                                                                                           \n\n"
        "                                                                      ..;===+.\n"
        "                                                                  .:=iiiiii=+=\n"
        "                                                               .=i))=;::+)i=+,\n"
        "                                                            ,=i);)I)))I):=i=;\n"
        "                                                         .=i==))))ii)))I:i++\n"
        "                                                       +)+))iiiiiiii))I=i+:'\n"
        "                                  .,:;;++++++;:,.       )iii+:::;iii))+i='\n"
        "                               .:;++=iiiiiiiiii=++;.    =::,,,:::=i));=+'\n"
        "                             ,;+==ii)))))))))))ii==+;,      ,,,:=i))+=:\n"
        "                           ,;+=ii))))))IIIIII))))ii===;.    ,,:=i)=i+\n"
        "                          ;+=ii)))IIIIITIIIIII))))iiii=+,   ,:=));=,\n"
        "                        ,+=i))IIIIIITTTTTITIIIIII)))I)i=+,,:+i)=i+\n"
        "                       ,+i))IIIIIITTTTTTTTTTTTI))IIII))i=::i))i='\n"
        "                      ,=i))IIIIITLLTTTTTTTTTTIITTTTIII)+;+i)+i`\n"
        "                      =i))IIITTLTLTTTTTTTTTIITTLLTTTII+:i)ii:'\n"
        "                     +i))IITTTLLLTTTTTTTTTTTTLLLTTTT+:i)))=,\n"
        "                     =))ITTTTTTTTTTTLTTTTTTLLLLLLTi:=)IIiii;\n"
        "                    .i)IIITTTTTTTTLTTTITLLLLLLLT);=)I)))))i;\n"
        "                    :))IIITTTTTLTTTTTTLLHLLLLL);=)II)IIIIi=:\n"
        "                    :i)IIITTTTTTTTTLLLHLLHLL)+=)II)ITTTI)i=\n"
        "                    .i)IIITTTTITTLLLHHLLLL);=)II)ITTTTII)i+\n"
        "                    =i)IIIIIITTLLLLLLHLL=:i)II)TTTTTTIII)i'\n"
        "                  +i)i)))IITTLLLLLLLLT=:i)II)TTTTLTTIII)i;\n"
        "                +ii)i:)IITTLLTLLLLT=;+i)I)ITTTTLTTTII))i;\n"
        "               =;)i=:,=)ITTTTLTTI=:i))I)TTTLLLTTTTTII)i;\n"
        "             +i)ii::,  +)IIITI+:+i)I))TTTTLLTTTTTII))=,\n"
        "           :=;)i=:,,    ,i++::i))I)ITTTTTTTTTTIIII)=+'\n"
        "         .+ii)i=::,,   ,,::=i)))iIITTTTTTTTIIIII)=+\n"
        "        ,==)ii=;:,,,,:::=ii)i)iIIIITIIITIIII))i+:'\n"
        "       +=:))i==;:::;=iii)+)=  `:i)))IIIII)ii+'\n"
        "     .+=:))iiiiiiii)))+ii;\n"
        "    .+=;))iiiiii)));ii+\n"
        "   .+=i:)))))))=+ii+\n"
        "  .;==i+::::=)i=;\n"
        "  ,+==iiiiii+,\n"
        "  `+=+++;`";

using namespace fys::mq;
using namespace fys::ws;
using namespace fys::network;

using AuthBusListener = BusListener <buslistener::Authenticator, FysBus<fys::pb::FySMessage, BUS_QUEUES_SIZE>>;

void welcome(bool verbose) {
    spdlog::set_async_mode(1024);
    spdlog::set_pattern("[%x %H:%M:%S] [%l] %v");

    std::vector<spdlog::sink_ptr> sinks;

    if (verbose) {
        auto stdout_sink = spdlog::sinks::stdout_sink_mt::instance();
        auto color_sink = std::make_shared<spdlog::sinks::ansicolor_sink>(stdout_sink);
        sinks.push_back(color_sink);
    }
    auto sys_logger = std::make_shared<spdlog::logger>("c", begin(sinks), end(sinks));
#ifdef DEBUG_LEVEL
    sys_logger->set_level(spdlog::level::debug);
#else
    sys_logger->set_level(spdlog::level::debug);
#endif
    spdlog::register_logger(sys_logger);
    sys_logger->info("Logger set to level {}", spdlog::get("c")->level());
    spdlog::get("c")->info(welcomeMsg);
}

int main(int argc, const char * const *argv) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    try {
        boost::asio::io_service ios;
        boost::asio::io_service::work work(ios);
        Context ctx(argc, argv);
        welcome(ctx.isVerbose());
        ctx.logContext();
        auto fysBus = std::make_shared<FysBus<fys::pb::FySMessage, BUS_QUEUES_SIZE> > (fys::pb::Type_ARRAYSIZE);
        WorldServer::ptr worldServer = WorldServer::create(ctx, ios, fysBus);
        buslistener::Authenticator authenticator(worldServer);
        AuthBusListener authenticatorListener(authenticator);

        authenticatorListener.launchListenThread(fysBus);
        ::sleep(1);
        worldServer->connectToGateway(ctx);
        worldServer->runPlayerAccept();
        ios.run();
    }
    catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}
