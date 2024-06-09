// место для вашего кода
#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"

using namespace std;

int main() {
    TransportCatalog::Transport::TransportCatalogue catalogue;

    int base_request_count;
    cin >> base_request_count >> ws;

    {
        TransportCatalog::Input::InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            string line;
            getline(cin, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

    int stat_request_count;
    cin >> stat_request_count >> ws;
    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(cin, line);
        TransportCatalog::Stat::ParseAndPrintStat(catalogue, line, cout);
    }
}