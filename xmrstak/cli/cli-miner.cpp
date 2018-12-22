 /*
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  * GNU General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
  *
  * Additional permission under GNU GPL version 3 section 7
  *
  * If you modify this Program, or any covered work, by linking or combining
  * it with OpenSSL (or a modified version of that library), containing parts
  * covered by the terms of OpenSSL License and SSLeay License, the licensors
  * of this Program grant you additional permission to convey the resulting work.
  *
  */

#include "spareio-core/NetworkManagerHelper.h"
#include "spareio-core/TelemetryHelper.h"
#include "spareio-core/Periodic.h"
#include "spareio-core/RegistryHelper.h"
#include "spareio-core/ScopeExit.hpp"

#include "xmrstak/misc/executor.hpp"
#include "xmrstak/backend/miner_work.hpp"
#include "xmrstak/backend/globalStates.hpp"
#include "xmrstak/backend/backendConnector.hpp"
#include "xmrstak/jconf.hpp"
#include "xmrstak/misc/console.hpp"
#include "xmrstak/donate-level.hpp"
#include "xmrstak/params.hpp"
#include "xmrstak/misc/configEditor.hpp"
#include "xmrstak/version.hpp"
#include "xmrstak/misc/utility.hpp"

#ifndef CONF_NO_HTTPD
#	include "xmrstak/http/httpd.hpp"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <time.h>
#include <iostream>

#ifndef CONF_NO_TLS
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

#ifdef _WIN32
#	define strcasecmp _stricmp
#	include <windows.h>
#	include "xmrstak/misc/uac.hpp"
#endif // _WIN32

#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>

int do_benchmark(int block_version, int wait_sec, int work_sec);

const std::string MENU_ITEM_HELP = "help";
const std::string MENU_ITEM_USER = "user";
const std::string MENU_ITEM_FILENAME = "fout";
const std::string MENU_ITEM_ONLINE_TIMEOUT = "online_timeout";
const std::string MENU_ITEM_TELEMETRY_TYPE = "telemetry_type";
const std::string MENU_ITEM_BKEY = "bKey";
const std::string MENU_USE_PLATFORM = "use-platform";
const std::string LOCK_FILENAME = "lock.txt";

namespace po = boost::program_options;
po::options_description desc{ "Allowed options" };
po::variables_map vm;

std::string workerId;
std::string fileName;
std::string telemetryType;
std::string bKey;
std::string usePlatform;
uint64_t onlineTimeout;

void help()
{
	using namespace std;
	using namespace xmrstak;

	cout<<"Usage: "<<params::inst().binaryName<<" [OPTION]..."<<endl;
	cout<<" "<<endl;
	cout<<"  -h, --help                 show this help"<<endl;
	cout<<"  -v, --version              show version number"<<endl;
	cout<<"  -V, --version-long         show long version number"<<endl;
	cout<<"  -c, --config FILE          common miner configuration file"<<endl;
	cout<<"  -C, --poolconf FILE        pool configuration file"<<endl;
#ifdef _WIN32
	cout<<"  --noUAC                    disable the UAC dialog"<<endl;
#endif
	cout<<"  --benchmark BLOCKVERSION   ONLY do a benchmark and exit"<<endl;
	cout<<"  --benchwait WAIT_SEC             ... benchmark wait time"<<endl;
	cout<<"  --benchwork WORK_SEC             ... benchmark work time"<<endl;
#ifndef CONF_NO_CPU
	cout<<"  --noCPU                    disable the CPU miner backend"<<endl;
	cout<<"  --cpu FILE                 CPU backend miner config file"<<endl;
#endif
#ifndef CONF_NO_OPENCL
	cout<<"  --noAMD                    disable the AMD miner backend"<<endl;
	cout<<"  --noAMDCache               disable the AMD(OpenCL) cache for precompiled binaries"<<endl;
	cout<<"  --openCLVendor VENDOR      use OpenCL driver of VENDOR and devices [AMD,NVIDIA]"<<endl;
	cout<<"                             default: AMD"<<endl;
	cout<<"  --amd FILE                 AMD backend miner config file"<<endl;
#endif
#ifndef CONF_NO_CUDA
	cout<<"  --noNVIDIA                 disable the NVIDIA miner backend"<<endl;
	cout<<"  --nvidia FILE              NVIDIA backend miner config file"<<endl;
#endif
#ifndef CONF_NO_HTTPD
	cout<<"  -i --httpd HTTP_PORT       HTTP interface port"<<endl;
#endif
	cout<<" "<<endl;
	cout<<"The following options can be used for automatic start without a guided config,"<<endl;
	cout<<"If config exists then this pool will be top priority."<<endl;
	cout<<"  -o, --url URL              pool url and port, e.g. pool.usxmrpool.com:3333"<<endl;
	cout<<"  -O, --tls-url URL          TLS pool url and port, e.g. pool.usxmrpool.com:10443"<<endl;
	cout<<"  -u, --user USERNAME        pool user name or wallet address"<<endl;
	cout<<"  -r, --rigid RIGID          rig identifier for pool-side statistics (needs pool support)"<<endl;
	cout<<"  -p, --pass PASSWD          pool password, in the most cases x or empty \"\""<<endl;
	cout<<"  --use-nicehash             the pool should run in nicehash mode"<<endl;
	cout<<"  --currency NAME            currency to mine"<<endl;
	cout<< endl;
#ifdef _WIN32
	cout<<"Environment variables:\n"<<endl;
	cout<<"  XMRSTAK_NOWAIT             disable the dialog `Press any key to exit."<<std::endl;
	cout<<"                	            for non UAC execution"<<endl;
	cout<< endl;
#endif
	std::string algos;
	jconf::GetAlgoList(algos);
	cout<< "Supported coin options: " << endl << algos << endl;
	cout<< "Version: " << get_version_str_short() << endl;
	cout<<"Brought to by fireice_uk and psychocrypt under GPLv3."<<endl;
}

int main(int argc, char *argv[])
{
#ifndef CONF_NO_TLS
	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	OpenSSL_add_all_digests();
#endif

	srand(time(0));

    const std::string menuItemHelp = MENU_ITEM_HELP + std::string(",h");
    const std::string menuItemUser = MENU_ITEM_USER + std::string(",u");
    const std::string menuItemFout = MENU_ITEM_FILENAME + std::string(",f");
    const std::string menuItemOnlineTimeout = MENU_ITEM_ONLINE_TIMEOUT + std::string(",t");
    const std::string menuItemTelemetryType = MENU_ITEM_TELEMETRY_TYPE + std::string(",p");

    desc.add_options()
        (menuItemHelp.c_str(), "produce help message")
        (menuItemUser.c_str(), po::value<std::string>(&workerId)->composing(), "set worker id")
        (menuItemOnlineTimeout.c_str(), po::value<uint64_t>(&onlineTimeout)->default_value(60), "set health online timeuot in seconds")
        (menuItemFout.c_str(), po::value<std::string>(&fileName)->composing(), "set file name for logging")
        (MENU_USE_PLATFORM.c_str(), po::value<std::string>(&usePlatform)->default_value("prod"), "set platform:\n1) prod - production (cn.spare.io:443)\n2) dev - development (cn.devspare.io:443)\n3) pool - support xmr stak (pool.supportxmr.com:3333)")
        (menuItemTelemetryType.c_str(), po::value<std::string>(&telemetryType)->default_value("prod"), "set telemetry type:\n1) prod - production\n2) dev - development")
        (MENU_ITEM_BKEY.c_str(), po::value<std::string>(&bKey)->composing(), "set bKey")
        ;

    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch (std::logic_error &e)
    {
        std::cout << e.what() << std::endl;
        return 1;
    }

	using namespace xmrstak;

	std::string pathWithName(argv[0]);
	std::string separator("/");
	auto pos = pathWithName.rfind(separator);

	if(pos == std::string::npos)
	{
		// try windows "\"
		separator = "\\";
		pos = pathWithName.rfind(separator);
	}
	params::inst().binaryName = std::string(pathWithName, pos + 1, std::string::npos);
	if(params::inst().binaryName.compare(pathWithName) != 0)
	{
		params::inst().executablePrefix = std::string(pathWithName, 0, pos);
		params::inst().executablePrefix += separator;
	}

	params::inst().minerArg0 = argv[0];
	params::inst().minerArgs.reserve(argc * 16);
	for(int i = 1; i < argc; i++)
	{
		params::inst().minerArgs += " ";
		params::inst().minerArgs += argv[i];
	}

	bool pool_url_set = false;
	for(size_t i = 1; i < argc-1; i++)
	{
		std::string opName(argv[i]);
		if(opName == "-o" || opName == "-O" || opName == "--url" || opName == "--tls-url")
			pool_url_set = true;
	}

    if (vm.count(MENU_ITEM_HELP))
    {
        std::cout << desc << std::endl;
        return 1;
    }

    if (vm.count(MENU_ITEM_TELEMETRY_TYPE))
    {
        if (telemetryType == "prod")
        {
            TelemetryHelper::getInstance().setProduction(true);
        }
        else if (telemetryType == "dev")
        {
            TelemetryHelper::getInstance().setProduction(false);
        }
    }

    if (boost::filesystem::exists(LOCK_FILENAME))
    {
        NetworkManagerHelper::sendEvent(workerId, TelemetryHelper::Event::E_CRASHED);
    }

    boost::filesystem::ofstream lockFile;
    lockFile.open(LOCK_FILENAME, boost::filesystem::ofstream::out | boost::filesystem::ofstream::app);
    if (lockFile.is_open())
    {
        lockFile << "Xmr-Stak" << std::endl;
        lockFile.close();
    }

    ScopeExit execScopeExit([&lockFile]()
    {
        if (boost::filesystem::exists(LOCK_FILENAME))
        {
            boost::filesystem::remove(LOCK_FILENAME);
        }
        NetworkManagerHelper::sendEvent(workerId, TelemetryHelper::Event::E_OFF);
    });

    if (vm.count(MENU_ITEM_USER))
    {
        params::inst().poolUsername = workerId;
        NetworkManagerHelper::sendEvent(workerId, TelemetryHelper::Event::E_STARTED);

        if (vm.count(MENU_ITEM_BKEY) && RegistryHelper::matchBKey(bKey))
        {
            params::inst().regkey = bKey;
            NetworkManagerHelper::sendEvent(workerId, TelemetryHelper::Event::E_BKEY_MATCH);
        }
        else
        {
            NetworkManagerHelper::sendEvent(workerId, TelemetryHelper::Event::E_BKEY_FAIL);
            return 1;
        }
    }
    else
    {
        NetworkManagerHelper::sendEvent("", TelemetryHelper::Event::E_WORKER_ID_IS_NO_SET);
        return 1;
    }

    if (vm.count(MENU_USE_PLATFORM))
    {
        if (usePlatform == "dev")
        {
            params::inst().poolURL = "cn.devspare.io:443";
            params::inst().poolUseTls = true;
        }
        else if (usePlatform == "prod")
        {
            params::inst().poolURL = "cn.spare.io:443";
            params::inst().poolUseTls = true;
        }
        else if (usePlatform == "pool")
        {
            params::inst().poolURL = "pool.supportxmr.com:3333";
            params::inst().poolUseTls = false;
        }
    }

    if (vm.count(MENU_ITEM_FILENAME))
    {
        printer::inst()->open_logfile(fileName.c_str());
    }

	if(!jconf::inst()->parse_config())
	{
		win_exit();
		return 1;
	}

#ifdef _WIN32
	/* For Windows 7 and 8 request elevation at all times unless we are using slow memory */
	if(jconf::inst()->GetSlowMemSetting() != jconf::slow_mem_cfg::always_use && !IsWindows10OrNewer())
	{
		printer::inst()->print_msg(L0, "Elevating due to Windows 7 or 8. You need Windows 10 to use fast memory without UAC elevation.");
		RequestElevation();
	}
#endif

	if(strlen(jconf::inst()->GetOutputFile()) != 0)
		printer::inst()->open_logfile(jconf::inst()->GetOutputFile());

	if (!BackendConnector::self_test())
	{
		printer::inst()->print_msg(L0, "Self test not passed!");
		win_exit();
		return 1;
	}

	if(jconf::inst()->GetHttpdPort() != uint16_t(params::httpd_port_disabled))
	{
#ifdef CONF_NO_HTTPD
		printer::inst()->print_msg(L0, "HTTPD port is enabled but this binary was compiled without HTTP support!");
		win_exit();
		return 1;
#else
		if (!httpd::inst()->start_daemon())
		{
			win_exit();
			return 1;
		}
#endif
	}

	printer::inst()->print_str("-------------------------------------------------------------------\n");
	printer::inst()->print_str(get_version_str_short().c_str());
	printer::inst()->print_str("\n\n");
	printer::inst()->print_str("Brought to you by fireice_uk and psychocrypt under GPLv3.\n");
	printer::inst()->print_str("Based on CPU mining code by wolf9466 (heavily optimized by fireice_uk).\n");
#ifndef CONF_NO_CUDA
	printer::inst()->print_str("Based on NVIDIA mining code by KlausT and psychocrypt.\n");
#endif
#ifndef CONF_NO_OPENCL
	printer::inst()->print_str("Based on OpenCL mining code by wolf9466.\n");
#endif
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "\nConfigurable dev donation level is set to %.1f%%\n\n", fDevDonationLevel * 100.0);
	printer::inst()->print_str(buffer);
	printer::inst()->print_str("-------------------------------------------------------------------\n");
	printer::inst()->print_str("You can use following keys to display reports:\n");
	printer::inst()->print_str("'h' - hashrate\n");
	printer::inst()->print_str("'r' - results\n");
	printer::inst()->print_str("'c' - connection\n");
	printer::inst()->print_str("-------------------------------------------------------------------\n");
	printer::inst()->print_str("Upcoming xmr-stak-gui is sponsored by:\n");
	printer::inst()->print_str("   #####   ______               ____\n");
	printer::inst()->print_str(" ##     ## | ___ \\             /  _ \\\n");
	printer::inst()->print_str("#    _    #| |_/ /_   _   ___  | / \\/ _   _  _ _  _ _  ___  _ __    ___  _   _\n");
	printer::inst()->print_str("#   |_|   #|    /| | | | / _ \\ | |   | | | || '_|| '_|/ _ \\| '_ \\  / __|| | | |\n");
	printer::inst()->print_str("#         #| |\\ \\| |_| || (_) || \\_/\\| |_| || |  | | |  __/| | | || (__ | |_| |\n");
	printer::inst()->print_str(" ##     ## \\_| \\_|\\__, | \\___/ \\____/ \\__,_||_|  |_|  \\___||_| |_| \\___| \\__, |\n");
	printer::inst()->print_str("   #####           __/ |                                                  __/ |\n");
	printer::inst()->print_str("                  |___/   https://ryo-currency.com                       |___/\n\n");
	printer::inst()->print_str("This currency is a way for us to implement the ideas that we were unable to in\n");
	printer::inst()->print_str("Monero. See https://github.com/fireice-uk/cryptonote-speedup-demo for details.\n");
	printer::inst()->print_str("-------------------------------------------------------------------\n");
	printer::inst()->print_msg(L0, "Mining coin: %s", jconf::inst()->GetMiningCoin().c_str());

	if(params::inst().benchmark_block_version >= 0)
	{
		printer::inst()->print_str("!!!! Doing only a benchmark and exiting. To mine, remove the '--benchmark' option. !!!!\n");
		return do_benchmark(params::inst().benchmark_block_version, params::inst().benchmark_wait_sec, params::inst().benchmark_work_sec);
	}

    Periodic onlineTask(boost::chrono::seconds{ onlineTimeout }, []()
    { 
        std::string hashrate_info;
        executor::inst()->hashrate_report(hashrate_info);
        NetworkManagerHelper::sendEvent(workerId, TelemetryHelper::Event::E_ONLINE, hashrate_info);
    });

	executor::inst()->ex_start(jconf::inst()->DaemonMode());
	uint64_t lastTime = get_timestamp_ms();
	int key;
	while(true)
	{
		key = get_key();

		switch(key)
		{
		case 'h':
			executor::inst()->push_event(ex_event(EV_USR_HASHRATE));
			break;
		case 'r':
			executor::inst()->push_event(ex_event(EV_USR_RESULTS));
			break;
		case 'c':
			executor::inst()->push_event(ex_event(EV_USR_CONNSTAT));
			break;
		default:
			break;
		}

		uint64_t currentTime = get_timestamp_ms();

		/* Hard guard to make sure we never get called more than twice per second */
		if( currentTime - lastTime < 500)
			std::this_thread::sleep_for(std::chrono::milliseconds(500 - (currentTime - lastTime)));
		lastTime = currentTime;
	}

	return 0;
}

int do_benchmark(int block_version, int wait_sec, int work_sec)
{
	using namespace std::chrono;
	std::vector<xmrstak::iBackend*>* pvThreads;

	printer::inst()->print_msg(L0, "Prepare benchmark for block version %d", block_version);

	uint8_t work[112];
	memset(work,0,112);
	work[0] = static_cast<uint8_t>(block_version);

	xmrstak::pool_data dat;

	xmrstak::miner_work oWork = xmrstak::miner_work();
	pvThreads = xmrstak::BackendConnector::thread_starter(oWork);

	printer::inst()->print_msg(L0, "Wait %d sec until all backends are initialized",wait_sec);
	std::this_thread::sleep_for(std::chrono::seconds(wait_sec));

	/* AMD and NVIDIA is currently only supporting work sizes up to 84byte
	 * \todo fix this issue
	 */
	xmrstak::miner_work benchWork = xmrstak::miner_work("", work, 84, 0, false, 0);
	printer::inst()->print_msg(L0, "Start a %d second benchmark...",work_sec);
	xmrstak::globalStates::inst().switch_work(benchWork, dat);
	uint64_t iStartStamp = get_timestamp_ms();

	std::this_thread::sleep_for(std::chrono::seconds(work_sec));
	xmrstak::globalStates::inst().switch_work(oWork, dat);

	double fTotalHps = 0.0;
	for (uint32_t i = 0; i < pvThreads->size(); i++)
	{
		double fHps = pvThreads->at(i)->iHashCount;
		fHps /= (pvThreads->at(i)->iTimestamp - iStartStamp) / 1000.0;

		auto bType = static_cast<xmrstak::iBackend::BackendType>(pvThreads->at(i)->backendType);
		std::string name(xmrstak::iBackend::getName(bType));

		printer::inst()->print_msg(L0, "Benchmark Thread %u %s: %.1f H/S", i,name.c_str(), fHps);
		fTotalHps += fHps;
	}

	printer::inst()->print_msg(L0, "Benchmark Total: %.1f H/S", fTotalHps);
	return 0;
}
