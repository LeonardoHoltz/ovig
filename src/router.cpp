#include "../include/router.hpp"

#include "osrm/match_parameters.hpp"
#include "osrm/nearest_parameters.hpp"
#include "osrm/route_parameters.hpp"
#include "osrm/table_parameters.hpp"
#include "osrm/trip_parameters.hpp"
#include "osrm/coordinate.hpp"
#include "osrm/json_container.hpp"
#include "osrm/status.hpp"

#include <exception>
#include <iostream>
#include <limits>

using namespace std;

void triangular_inequality(std::vector< std::vector<double> >& matrix);

Router::Router(string osrm_data) {
	// OSRM data file and no data in shared mem.
	config.storage_config = {osrm_data};
    config.use_shared_memory = false;

	// Available routing speed up techniques:
    // - Contraction Hierarchies (CH): requires extract+contract pre-processing
    // - Multi-Level Dijkstra (MLD): requires extract+partition+customize pre-processing
    // config.algorithm = EngineConfig::Algorithm::CH;
    config.algorithm = osrm::EngineConfig::Algorithm::MLD;
	osrm = new osrm::OSRM{config};
}

void Router::route(const std::vector<GeoLocation>& locations, std::vector< std::vector<double> >& matrix){
	table_route(locations, matrix);	
	triangular_inequality(matrix);
}

void triangular_inequality(std::vector< std::vector<double> >& matrix){
	for(size_t i=0;i<matrix.size();i++){
		for(size_t j=0;j<matrix.size();j++){
			for(size_t k=0;k<matrix.size();k++){
				if(matrix[i][j] + matrix[j][k] < matrix[i][k]) matrix[i][k] = matrix[i][j]+matrix[j][k];
			}
		}
	}
}

void Router::table_route(const std::vector<GeoLocation> &locations, std::vector< std::vector<double> >& matrix){
	osrm::TableParameters params;
	for(GeoLocation loc : locations){
		params.coordinates.push_back({osrm::util::FloatLongitude{loc.lon}, osrm::util::FloatLatitude{loc.lat}});
	}
	
	osrm::json::Object result;
	const auto status = osrm->Table(params, result);
	if (status == osrm::Status::Ok){
		auto &mat = result.values["durations"].get<osrm::json::Array>();
		for(size_t i=0;i<locations.size();i++){
			for(size_t j=0;j<locations.size();j++){
				if(i == j) continue;
				auto &row = mat.values.at(i).get<osrm::json::Array>();
				matrix[i][j] = std::max(0.0, std::ceil(row.values.at(j).get<osrm::json::Number>().value/60.0));
			}
		}
	}else if (status == osrm::Status::Error){
		//TODO: check what to do in case of error
		const auto code = result.values["code"].get<osrm::json::String>().value;
		const auto message = result.values["message"].get<osrm::json::String>().value;			
		std::cout << "Code: " << code << "\n";
		std::cout << "Message: " << code << "\n";
		throw "Dead end error.\n";
		exit(EXIT_FAILURE);
	}
}