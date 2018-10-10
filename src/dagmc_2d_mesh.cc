// moab includes
#include "moab/Core.hpp"
#include "MBTagConventions.hpp"
#include "vertex_inserter.hh"
#include "dagmc_slicer.hh"

#include <fstream>
#include <list>
#include "boost/program_options.hpp"

#include "CGAL/Polygon_mesh_slicer.h"
#include "CGAL/Constrained_Delaunay_triangulation_2.h"
#include "CGAL/Triangulation_face_base_with_info_2.h"

#include <CGAL/Partition_traits_2.h>
#include <CGAL/Partition_is_valid_traits_2.h>
#include <CGAL/polygon_function_objects.h>
#include <CGAL/partition_2.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/random_polygon_2.h>

#include "CGAL/Polygon_2.h"
#include "CGAL/draw_triangulation_2.h"
#include "CGAL/Delaunay_mesher_2.h"
#include "CGAL/Delaunay_mesh_face_base_2.h"
#include "CGAL/Delaunay_mesh_size_criteria_2.h"
//#include "CGAL/Partition_traits_2.h"
//#include "CGAL/partition_2.h"

struct FaceInfo2 {
  FaceInfo2(){}
  int nesting_level;
  bool in_domain(){
    return nesting_level%2 == 1;
  }
};

typedef CGAL::Exact_predicates_inexact_constructions_kernel K2;

typedef CGAL::Triangulation_vertex_base_2<K2>                      Vb;
typedef CGAL::Triangulation_face_base_with_info_2<FaceInfo2,K2>    Fbb;
typedef CGAL::Constrained_triangulation_face_base_2<K2,Fbb>        Fb;
typedef CGAL::Triangulation_data_structure_2<Vb,Fb>               TDS;
typedef CGAL::Exact_predicates_tag                                Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<K2, TDS, Itag>  CDT;

//typedef CGAL::Polygon_2<K2> Polygon_2;
typedef CGAL::Partition_traits_2<K2> Traits;
typedef Traits::Polygon_2 Polygon_2;

typedef CDT::Face_handle Face_handle;
typedef CDT::Vertex_handle Vertex_handle;
typedef CDT::Point Point;

typedef std::list<Polygon_2> Polygon_list;

moab::Tag idtag;

void mark_domains(CDT& ct, 
                  CDT::Face_handle start, 
                  int index, 
                  std::list<CDT::Edge>& border ) {
  if(start->info().nesting_level != -1){
    return;
  }
  std::list<CDT::Face_handle> queue;
  queue.push_back(start);
  while(! queue.empty()){
    CDT::Face_handle fh = queue.front();
    queue.pop_front();
    if(fh->info().nesting_level == -1){
      fh->info().nesting_level = index;
      for(int i = 0; i < 3; i++){
        CDT::Edge e(fh,i);
        CDT::Face_handle n = fh->neighbor(i);
        if(n->info().nesting_level == -1){
          if(ct.is_constrained(e)) border.push_back(e);
          else queue.push_back(n);
        }
      }
    }
  }
}
//explore set of facets connected with non constrained edges,
//and attribute to each such set a nesting level.
//We start from facets incident to the infinite vertex, with a nesting
//level of 0. Then we recursively consider the non-explored facets incident 
//to constrained edges bounding the former set and increase the nesting level by 1.
//Facets in the domain are those with an odd nesting level.
void mark_domains(CDT& cdt) {
  for(CDT::All_faces_iterator it = cdt.all_faces_begin(); it != cdt.all_faces_end(); ++it){
    it->info().nesting_level = -1;
  }
  std::list<CDT::Edge> border;
  mark_domains(cdt, cdt.infinite_face(), 0, border);
  while(! border.empty()){
    CDT::Edge e = border.front();
    border.pop_front();
    CDT::Face_handle n = e.first->neighbor(e.second);
    if(n->info().nesting_level == -1){
      mark_domains(cdt, n, e.first->info().nesting_level+1, border);
    }
  }
}

// stride through the slices and make a triangulation
void make_2d_triangulation(const std::map<int,Polylines> slices) {
  // loop over the slices
  //  std::vector<Polygon_2> polygons;

  moab::Core *MOAB = new moab::Core();
  VertexInserter::VertexInserter *vi = new VertexInserter::VertexInserter(MOAB);

  moab::ErrorCode rval = moab::MB_FAILURE;
  rval = MOAB->tag_get_handle("ID_TAG", 1, moab::MB_TYPE_INTEGER, idtag, moab::MB_TAG_SPARSE | moab::MB_TAG_CREAT);
  MB_CHK_SET_ERR_RET(rval, "Couldnt get geom dim tag");
  
  // loop over the slices
  for ( std::pair<int,Polylines> boundary : slices ) {
    std::cout << "id: " << boundary.first;// << std::endl;
    std::cout << " size:" << boundary.second.size() << std::endl;
    int id = boundary.first;
    //if ( id == 3 ) { break;}
    //if ( id != 2 ) { continue;}
    std::cout << std::scientific;
    std::cout << std::setprecision(18);// << std::endl;
 
    CDT cdt;

    // for each closed loop in the boundary 
    for ( Polyline_type segments : boundary.second ) {
      // for each point in a given loop
      Polygon_2 polygon;
      //K2::Point_3 last_point;
      double x_old,y_old = 0.0;
      segments.pop_back();
      for ( K2::Point_3 point : segments ) {
        double x_new = CGAL::to_double(point.x());
        double y_new = CGAL::to_double(point.y());
        //std::cout << x_new << " " << y_new << std::endl;
        //std::cout << " segment " << std::endl;
        if ( x_new != x_old || y_new != y_old ) {
          polygon.push_back(Point(point.x(),point.y()));
          //polygon.push_back(Point(x_new,y_old));
          //cdt.insert(Point(point.x(),point.y()));
        } 
        x_old = x_new;
        y_old = y_new;
        //std::cout << polygon << std::endl;
        std::cout << "simple?: " << polygon.is_simple() << std::endl;
        std::cout << "convex?: " << polygon.is_convex() << std::endl;
        //std::cout << x_old << " " << y_old << std::endl;
//        last_point = point;
      }
      // due to the fact this this polygon could be 
      // convex we need to partition and then 
      // triangulate each partition
      
      Polygon_list poly_parts;
      CGAL::Partition_traits_2<K2> partition_traits;
      // generate partition for polygon
      CGAL::approx_convex_partition_2(polygon.vertices_begin(),
                                      polygon.vertices_end(),
                                      std::back_inserter(poly_parts));
      //std::cout << polygon << std::endl;
      /*
      CGAL::optimal_convex_partition_2(polygon.vertices_begin(),
                                       polygon.vertices_end(),
                                       std::back_inserter(poly_parts),
                                       partition_traits);
                                       */
      std::cout << poly_parts.size() << std::endl;
      //std::cout << polygon << std::endl;
      // append the polygon to the vector
      //polygons.push_back(polygon);    

      for (Polygon_2 poly : poly_parts) {
        // std::cout << poly << std::endl;
        cdt.insert_constraint(poly.vertices_begin(),
			                        poly.vertices_end(),
                              true);
      }

      mark_domains(cdt);

      CDT::Finite_faces_iterator t; // = cdt.faces_begin();

      // CGAL::draw(cdt);
/*
     for(CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
      fit != cdt.finite_faces_end(); ++fit) {
        if(fit->info().is_in_domain()) std::cout << "rah" << std::endl;
      }
*/
      int count = 0;
      for ( t = cdt.finite_faces_begin() ;
           t != cdt.finite_faces_end() ;
           ++t ) {

        if(!(t->info().in_domain())) continue;
        
        count++;
        moab::EntityHandle triangle;
        std::array<moab::EntityHandle,3> connectivity;
        for ( int i = 0 ; i < 3 ; i++ ) {

          double x = CGAL::to_double(t->vertex(i)->point().hx());
          double y = CGAL::to_double(t->vertex(i)->point().hy());
        
          std::array<double,3> coords = {x,y,0.0};
          // make new moab vertex
          rval = vi->insert_vertex(coords,connectivity[i]);
        }
      // make a new triangle
      rval = MOAB->create_element(moab::MBTRI,&(connectivity[0]),3,triangle);
      // tag the triangle
      const void* vol_id = &id;
      rval = MOAB->tag_set_data(idtag,&triangle,1,&vol_id);
      }
    }
    //break;
    std::cout << id << std::endl;
  }
  rval = MOAB->write_mesh("mesh.h5m");
  delete vi;
  delete MOAB;
}

int main(int argc, char* argv[]) {

  moab::Core *MBI = new moab::Core();
  
  moab::ErrorCode rval = moab::MB_FAILURE;
  moab::EntityHandle input_set;

  boost::program_options::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("input-file", boost::program_options::value<std::string>(), "input file")
    ("slice-position", boost::program_options::value< std::vector<double> >()->multitoken(), "slice_position")
    ;

  boost::program_options::positional_options_description p;
  p.add("input_file", -1);

  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::command_line_parser(argc,argv).
	    options(desc).positional(p).run(),vm);
  boost::program_options::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return -1;
  }

  std::string filename = "";
  if (vm.count("input-file")) {
    filename = vm["input-file"].as<std::string>();
  } else {
    std::cout << "input-file not specified" << std::endl;
    return -1;
  }

  std::vector<double> slice_position;
  if (vm.count("slice-position")) {
    slice_position = vm["slice-position"].as< std::vector<double> >();
    if(slice_position.size() != 3) {
      std::cout << "There needs to three elements" << std::endl;
      return -1;
    }
  } else {
    std::cout << "slice-position not specified" << std::endl;
    return -1;
  }

  std::cout << "open file " << filename << std::endl;
  std::cout << "slicing at " << slice_position[0] << " " << slice_position[1];
  std::cout << " " << slice_position[2] << std::endl;
  
  rval = MBI->create_meshset(moab::MESHSET_SET, input_set); 
  MB_CHK_SET_ERR(rval, "failed to create meshset");
  std::cout << "Loading input file..." << std::endl;
  rval = MBI->load_file(filename.c_str(), &input_set);  
  
  // make the cgal geometry
  MOABInterface *cgal = new MOABInterface(MBI);
  cgal->makeCGALGeometry(); 

  std::map<int,Polylines> slices;

  double dir[3] = {0.,0.,1.};
  slices = cgal->sliceGeometry(dir,slice_position[1]);

  make_2d_triangulation(slices);
  
  return 0;

}
