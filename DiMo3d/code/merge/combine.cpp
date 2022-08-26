#include<stdio.h>
#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include<algorithm>
#include<cmath>

#include <algorithm>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

//#include"hash.h"
//#include"readini.h"

using namespace std;


#define DEBUG 0;

// CONSTANTS
const int overlapdim = 3;

//CLASSES


//STRUCTS
class point{
	public:
		int x,y,z;
		double v;
		bool in_bbox;
};

// Edge pair
class cp{
	public:
		int p1,p2;
		void Reorder(){
			if (p1>p2) swap(p1, p2);
		}
};


// Triangle pair
class tp{
	public:
		int p1,p2,p3;
		void Reorder(){
			if (p1>p2) swap(p1, p2);
			if (p1>p3) swap(p1, p3);
			if (p2>p3) swap(p2, p3);
		}
};


class VertexHash{
        private:
                boost::unordered_map<point, int> v_hash;
                //boost::unordered_map<point, int> v_hash;

        public:
                VertexHash(){
                        v_hash.clear();
                }

                int GetIndex(point p)
                {
        			if (v_hash.count(p) > 0)
        	        	return v_hash[p];
    	    		else
	                	return -1;
                }
                
                void InsertVertex(point p, int n)
                {
        			v_hash.insert(std::make_pair(p, n));
				}


                int size(){return v_hash.size();}
                boost::unordered_map<point, int>::iterator begin(){
                        return v_hash.begin();
                }


                boost::unordered_map<point, int>::iterator end(){
                        return v_hash.end();
                }
};


std::size_t hash_value(const point &pt){
        std::size_t seed = 0;
        boost::hash_combine(seed, pt.x);
        boost::hash_combine(seed, pt.y);
        boost::hash_combine(seed, pt.z);
        return seed;
}

bool operator==(const point &a, const point &b)
{
        return a.x == b.x && a.y == b.y && a.z == b.z;
}



class EdgeHash{
        // Assuming vertex are given in ascending order
        private:
                boost::unordered_set<cp> e_hash;

        public:
                EdgeHash(){
                        e_hash.clear();
                }
                bool HasEdge(cp edge)
                {
                	return e_hash.count(edge) > 0;
                }
                void InsertEdge(cp edge)
                {
                	e_hash.insert(edge);
                }

                int size(){return e_hash.size();}
                boost::unordered_set<cp>::iterator begin(){return e_hash.begin();}
                boost::unordered_set<cp>::iterator end(){return e_hash.end();}
};


std::size_t hash_value(const cp &e){
        std::size_t seed = 0;
        boost::hash_combine(seed, e.p1);
        boost::hash_combine(seed, e.p2);
        return seed;
}

bool operator==(const cp &a, const cp &b)
{
        return a.p1 == b.p1 && a.p2 == b.p2;
}

class TriangleHash{
        // Assuming vertex are given in ascending order
        private:
                boost::unordered_set<tp> t_hash;
        public:
                TriangleHash(){
                        t_hash.clear();
                }
                bool HasTriangle(tp triangle)
                {
                	return t_hash.count(triangle) > 0;
                }
                void InsertTriangle(tp triangle)
                {
                	t_hash.insert(triangle);
                }
                int size(){return t_hash.size();}
                boost::unordered_set<tp>::iterator begin(){return t_hash.begin();}
                boost::unordered_set<tp>::iterator end(){return t_hash.end();}
};


std::size_t hash_value(const tp &t){
        std::size_t seed = 0;
        boost::hash_combine(seed, t.p1);
        boost::hash_combine(seed, t.p2);
        boost::hash_combine(seed, t.p3);
        return seed;
}

bool operator==(const tp &a, const tp &b)
{
        return a.p1 == b.p1 && a.p2 == b.p2 && a.p3 == b.p3;
}


struct parameter{
	string out_prefix;
	int overlap[overlapdim];
};

struct fileinfo{
	string name;
	// pixel coordinate
	// 2 pairs of 3D coordinate defines the bounding box
	// offset: xmin ymin zmin xmax ymax zmax
	int offset[6];
};


//VARIABLES
vector<point> vertex;
vector<cp> edge;
vector<tp> triangle;

VertexHash vh;
EdgeHash eh;
TriangleHash th;

vector<point> graph_vert;


double norm_const = 1;
int vcount = 0;
int nb = 12;
string persistence_threshold;



//FUNCTIONS
int init_file(const string &inputname,
			  parameter &para,
			  vector<fileinfo> &blocks)
{
	fstream ifs(inputname.c_str(), ios::in);
	// get filename
	ifs >> para.out_prefix;
	
	// get overlap
	for(int i = 0; i < 3; i++){
		ifs >> para.overlap[i];
	}
	
	// get block info
	blocks.clear();
	while(!ifs.eof()){
		fileinfo block_file;
		ifs >> block_file.name;
		if (ifs.eof()) break;
		// should detect data error?

		for(int i = 0; i < 6; i++)
		{
			ifs >> block_file.offset[i];
			//cout << block_file.offset[i] << endl;
		}
		blocks.push_back(block_file);
	}
	ifs.close();
	return 0;
}

string getpath(const string &filename){
        int pathstop = filename.find_last_of("/\\");
        string path = filename.substr(0, pathstop);
        if (path.size() > 0) path = path + "/";
        cout << "input path" << endl;
        return path;
}


int adjust_bbox(vector<fileinfo> &blocks, parameter para)
{
	for(int i = 0; i < blocks.size(); ++i){
		// original order: xmin,ymin,zmin,xmax,ymax,zmax
		// make bbox in order: xmin, xmax, ymin, ymax, zmin, zmax
		swap(blocks[i].offset[1], blocks[i].offset[3]);
		swap(blocks[i].offset[2], blocks[i].offset[4]);
		swap(blocks[i].offset[2], blocks[i].offset[3]);
		
		
		cout << "before adjust: " << blocks[i].offset[0] << " " << blocks[i].offset[1] << " " << blocks[i].offset[2] << " " << blocks[i].offset[3] << " " << blocks[i].offset[4] << " " << blocks[i].offset[5] << endl; 
		for(int j = 0; j < overlapdim; ++j)
		{
			//TODO what does this do?
			// shrink axis-min
			blocks[i].offset[j*2] += para.overlap[j];
			// shrink axis-max
			blocks[i].offset[j*2+1] -= para.overlap[j];
		}
		
		cout << "after adjust: " << blocks[i].offset[0] << " " << blocks[i].offset[1] << " " << blocks[i].offset[2] << " " << blocks[i].offset[3] << " " << blocks[i].offset[4] << " " << blocks[i].offset[5] << endl; 
	}
	return 0;
}

bool in_range(point p, const vector<int> &bbox){
	if (p.x >= bbox[0] && p.x <= bbox[1] &&
		p.y >= bbox[2] && p.y <= bbox[3] &&
		p.z >= bbox[4] && p.z <= bbox[5])
	{
	   return true;
	}
	else{
		return false;
	}
}

void update_bbox(vector<double> &vb, bool &first, point p)
{
	if (first){
		first = 0;
		vb[0] = p.x; vb[1] = p.x;
		vb[2] = p.y; vb[3] = p.y;
		vb[4] = p.z; vb[5] = p.z;
	}else{
		vb[0] = p.x < vb[0]?p.x:vb[0]; vb[1] = p.x > vb[1]?p.x:vb[1];
				vb[2] = p.y < vb[2]?p.y:vb[2]; vb[3] = p.y > vb[3]?p.y:vb[3];
				vb[4] = p.z < vb[4]?p.z:vb[4]; vb[5] = p.z > vb[5]?p.z:vb[5];
	}
}

double kernel_init(double sigma)
{
	double sum = 0;
	for (int i = -1; i <= 1; ++i)
		for(int j = -1; j <= 1; ++j){
			for(int k = -1; k <= 1; ++k){
				sum += exp(- (i*i + j*j +k*k)/(2* sigma * sigma));
				// cout << exp(- (i*i + j*j +k*k)/(2* sigma * sigma)) << " ";
			}
		// cout << "\n";
	}
	return 1.0/sum;
}

int diffuse(point p){
	double sigma = 1;
	int sum = 0;
	for (int i = -2; i <= 2; ++i)
		for(int j = -2; j <= 2; ++j)
			for(int k = -2; k <= 2; ++k){
				point dif_p;
				
				dif_p.x = p.x + i;
				dif_p.y = p.y + j;
				dif_p.z = p.z + k;
				dif_p.v = p.v * norm_const * exp(- (i*i + j*j +k*k)/(2* sigma * sigma));
				dif_p.in_bbox = false;
				/*if (dif_p.v < 1e-8) {
					continue;
				}*/
				
				int idx = vh.GetIndex(dif_p);
				if (idx < 0){
					dif_p.in_bbox = false;
					vh.InsertVertex(dif_p, vcount);
					vertex.push_back(dif_p);
					vcount++;
				}else{
					vertex[idx].v += dif_p.v;
					vertex[idx].in_bbox = false;
				}
	}
	return sum;
}


void ProcessGraph(fileinfo blk)
{
	string vert_name = blk.name + "/" + persistence_threshold +  "/dimo_vert.txt";
	string edge_name = blk.name + "/" + persistence_threshold +  "/dimo_edge.txt";
	
	fstream fp(vert_name.c_str(), ios::in);
	if (fp.fail()){
		cout << "Cannot open "<<vert_name << endl;
		return;
	}
	
	int nl;
	graph_vert.clear();
	bool first = 1;
	vector<double> vertexbound(6, 0);
	
	string input_str;
	getline(fp, input_str);
	while(!fp.eof()){
		point p;
		sscanf(input_str.c_str(), 
			   "%d%d%d%lf",
			   &p.x, &p.y, &p.z, &p.v);
		//cout << input_str << endl;
		//cout << vector<int>({p.x, p.y, p.z, p.v}) << endl;
		//cout << p.v << "    ";
		p.v = -p.v;
		//cout << p.v << endl;
		graph_vert.push_back(p);  // + 1 or not
		update_bbox(vertexbound, first, p);
		getline(fp, input_str);
	}
	fp.close();
	printf("\tRead %d vertices\n \tbounded in:", graph_vert.size());
	for(int i =0; i< 6; i++) printf(" %.0f ", vertexbound[i]);
	printf("\n");
	
	
	fp.open(edge_name.c_str(), ios::in);
	int e1, e2;
	int counter = 0;
	int c_interior = 0;
	//double persist;

	//cout << "check 1" << endl;

	//int count = 0;

	
	vector<int> bbox(blk.offset, blk.offset + 6);
	cout << "bbox: " << bbox[0] << " " << bbox[1] << " " << bbox[2] << " " << bbox[3] << " " << bbox[4] << " " << bbox[5] << endl;
	cout << "blk.offset: " << blk.offset[0] << " " << blk.offset[1] << " " << blk.offset[2] << " " << blk.offset[3] << " " << blk.offset[4] << " " << blk.offset[5] << endl;

	getline(fp, input_str);
	while(!fp.eof()){
		//count++;
		//cout << count << endl;
		//sscanf(input_str.c_str(), "%d%d%d%lf", &e1, &e2, &nl,&persist);
		sscanf(input_str.c_str(), "%d%d", &e1, &e2);
		//e1--;e2--;
		if (in_range(graph_vert[e1], bbox) && in_range(graph_vert[e2], bbox)){
			// only insert a point.
			int idx = vh.GetIndex(graph_vert[e1]);
			int idx1 = -1, idx2 = -1;
			if (idx < 0){
				graph_vert[e1].in_bbox = true;
				vh.InsertVertex(graph_vert[e1], vcount);
				vertex.push_back(graph_vert[e1]);
				idx1 = vcount;
				vcount++;
			}else{
				vertex[idx].v += graph_vert[e1].v;
				idx1 = idx;
			}
			
			idx = vh.GetIndex(graph_vert[e2]);
			if (idx < 0){
				graph_vert[e2].in_bbox = true;
				vh.InsertVertex(graph_vert[e2], vcount);
				vertex.push_back(graph_vert[e2]);
				idx2 = vcount;
				vcount++;
			}else{
				vertex[idx].v += graph_vert[e2].v;
				idx2 = idx;
			}

			cp new_edge;
			new_edge.p1 = idx1; new_edge.p2 = idx2;
			new_edge.Reorder();
			if (!eh.HasEdge(new_edge)){
				edge.push_back(new_edge);
				eh.InsertEdge(new_edge);
			}
			c_interior++;
		}	
		else{
			if (!in_range(graph_vert[e1], bbox)){
				diffuse(graph_vert[e1]);
				counter ++;
			}
			if (!in_range(graph_vert[e2], bbox)){
				diffuse(graph_vert[e2]);
				counter ++;
			}
		}
		getline(fp, input_str);
	}
	
	printf("\tprocessed %d interior edges, %d diffused points\n", c_interior, counter, vh.size());
	fp.close();
	printf("\tdone\n");
}

int triangle_cube(int i, int j, int k, int AB){
	if (AB == 0){ // triangles of Type A
		int TypeAtri[3*16][3] = {{0,0,0}, {1,0,1}, {0,0,1},
						  {0,0,0}, {1,0,0}, {1,0,1},
						  {0,0,0}, {0,1,1}, {0,1,0},
						  {0,0,0}, {0,0,1}, {0,1,1},
						  {0,0,0}, {1,1,0}, {1,0,0},
						  {0,0,0}, {0,1,0}, {1,1,0},
						  {1,0,0}, {1,1,0}, {1,0,1},
						  {1,0,1}, {1,1,0}, {1,1,1},
						  {0,1,0}, {0,1,1}, {1,1,0},
						  {0,1,1}, {1,1,0}, {1,1,1},
						  {0,0,1}, {0,1,1}, {1,0,1},
						  {0,1,1}, {1,1,1}, {1,0,1},
						  {0,0,0}, {0,1,1}, {1,0,1},// fill
						  {0,0,0}, {1,0,1}, {1,1,0},
						  {1,1,0}, {1,0,1}, {0,1,1},
						  {0,0,0}, {1,1,0}, {0,1,1}
				   };
		for (int cnt = 0; cnt < nb; cnt++){
			int sub1, sub2, sub3;

			point p;
			p.x = i + TypeAtri[cnt*3][0];
			p.y = j + TypeAtri[cnt*3][1];
			p.z = k + TypeAtri[cnt*3][2];
			sub1 = vh.GetIndex(p);
			if (sub1 < 0){
				p.v = 1e-6;
				vh.InsertVertex(p, vcount);
				sub1 = vcount;
				vcount++;
				vertex.push_back(p);
			}
			
			p.x = i + TypeAtri[cnt*3+1][0];
			p.y = j + TypeAtri[cnt*3+1][1];
			p.z = k + TypeAtri[cnt*3+1][2];
			sub2 = vh.GetIndex(p);
			if (sub2 < 0){
				p.v = 1e-6;
				vh.InsertVertex(p, vcount);
				sub2 = vcount;
				vcount++;
				vertex.push_back(p);
			}

			p.x = i + TypeAtri[cnt*3+2][0];
			p.y = j + TypeAtri[cnt*3+2][1];
			p.z = k + TypeAtri[cnt*3+2][2];
			sub3 = vh.GetIndex(p);
			if (sub3 < 0){
				p.v = 1e-6;
				vh.InsertVertex(p, vcount);
				sub3 = vcount;
				vcount++;
				vertex.push_back(p);
			}
			
			tp new_triangle;
			new_triangle.p1 = sub1; new_triangle.p2 = sub2; new_triangle.p3 = sub3;
			new_triangle.Reorder();
			if (!th.HasTriangle(new_triangle)){
				triangle.push_back(new_triangle);
				th.InsertTriangle(new_triangle);
			}

			cp new_edge1;
			new_edge1.p1 = sub1; new_edge1.p2 = sub2;
			new_edge1.Reorder();
			cp new_edge2;
			new_edge2.p1 = sub1; new_edge2.p2 = sub3;
			new_edge2.Reorder();
			cp new_edge3;
			new_edge3.p1 = sub2; new_edge3.p2 = sub3;
			new_edge3.Reorder();

			//if (DEBUG){printf("\t%d %d %d\n", sub1, sub2, sub3);}

			if (!eh.HasEdge(new_edge1)){
				edge.push_back(new_edge1);
				eh.InsertEdge(new_edge1);
			}
			if (!eh.HasEdge(new_edge2)){
				edge.push_back(new_edge2);
				eh.InsertEdge(new_edge2);
			}
			if (!eh.HasEdge(new_edge3)){
				edge.push_back(new_edge3);
				eh.InsertEdge(new_edge3);
			}
		}
	}
	else{// triangles of Type B
		int TypeAtri[3*16][3] = {{0,0,0}, {0,0,1}, {0,1,0},
							   {0,0,0}, {0,1,0}, {1,0,0},
							   {0,0,0}, {1,0,0}, {0,0,1},
							   {0,1,1}, {0,1,0}, {0,0,1},
							   {0,1,0}, {1,1,0}, {1,0,0},
							   {0,0,1}, {1,0,0}, {1,0,1},
							   {0,1,1}, {1,1,1}, {0,0,1},
							   {0,0,1}, {1,0,1}, {1,1,1},
							   {1,0,1}, {1,1,1}, {1,0,0},
							   {1,0,0}, {1,1,0}, {1,1,1},
							   {0,1,1}, {1,1,1}, {0,1,0},
							   {0,1,0}, {1,1,1}, {1,1,0},
							   {0,0,1}, {0,1,0}, {1,0,0},// fill
							   {0,1,0}, {1,0,0}, {1,1,1},
							   {0,0,1}, {1,1,1}, {1,0,0},
							   {0,0,1}, {1,1,1}, {0,1,0}
							   };
		for (int cnt = 0; cnt < nb; cnt++){
			int sub1, sub2, sub3;

			point p;
			p.x = i + TypeAtri[cnt*3][0];
			p.y = j + TypeAtri[cnt*3][1];
			p.z = k + TypeAtri[cnt*3][2];
			sub1 = vh.GetIndex(p);
			if (sub1 < 0){
				p.v = 1e-6;
				vh.InsertVertex(p, vcount);
				sub1 = vcount;
				vcount++;
				vertex.push_back(p);
			}
			
			p.x = i + TypeAtri[cnt*3+1][0];
			p.y = j + TypeAtri[cnt*3+1][1];
			p.z = k + TypeAtri[cnt*3+1][2];
			sub2 = vh.GetIndex(p);
			if (sub2 < 0){
				p.v = 1e-6;
				vh.InsertVertex(p, vcount);
				sub2 = vcount;
				vcount++;
				vertex.push_back(p);
			}

			p.x = i + TypeAtri[cnt*3+2][0];
			p.y = j + TypeAtri[cnt*3+2][1];
			p.z = k + TypeAtri[cnt*3+2][2];
			sub3 = vh.GetIndex(p);
			if (sub3 < 0){
				p.v = 1e-6;
				vh.InsertVertex(p, vcount);
				sub3 = vcount;
				vcount++;
				vertex.push_back(p);
			}
			
			tp new_triangle;
			new_triangle.p1 = sub1; new_triangle.p2 = sub2; new_triangle.p3 = sub3;
			new_triangle.Reorder();
			if (!th.HasTriangle(new_triangle)){
				triangle.push_back(new_triangle);
				th.InsertTriangle(new_triangle);
			}

			cp new_edge1;
			new_edge1.p1 = sub1; new_edge1.p2 = sub2;
			new_edge1.Reorder();
			cp new_edge2;
			new_edge2.p1 = sub1; new_edge2.p2 = sub3;
			new_edge2.Reorder();
			cp new_edge3;
			new_edge3.p1 = sub2; new_edge3.p2 = sub3;
			new_edge3.Reorder();

			//if (DEBUG){printf("\t%d %d %d\n", sub1, sub2, sub3);}

			if (!eh.HasEdge(new_edge1)){
				edge.push_back(new_edge1);
				eh.InsertEdge(new_edge1);
			}
			if (!eh.HasEdge(new_edge2)){
				edge.push_back(new_edge2);
				eh.InsertEdge(new_edge2);
			}
			if (!eh.HasEdge(new_edge3)){
				edge.push_back(new_edge3);
				eh.InsertEdge(new_edge3);
			}
		}
	}
	return 0;
}

int Triangulate(){
	int original_total = vertex.size();
	
	double THD = 1e-6;
	int skip_count = 0;
	for(int v = 0; v < original_total; ++v){
		int i, j, k, val;
		
		if (v%10000==0){
			cout << '\r';
			cout << v << '/' << original_total;
			cout.flush();
		}
		
		i = vertex[v].x; j = vertex[v].y; k = vertex[v].z; val = vertex[v].v;
		if (vertex[v].in_bbox) {
			// cout << "skipped something\n";
			skip_count++;
			continue;
		}

		if ((i+j+k)%2==1){
			triangle_cube(i, j, k, 0);
		}
		else{
			triangle_cube(i, j, k, 1);
		}
	}
	
	printf("\tSkipped %d points\n", skip_count);
	return 0;
}

void simplex_output(string fname){
    string binname = fname + ".sc";
    ofstream ofs(binname,ios::binary);
    cout << "writing " << vertex.size() << " vertex to " << fname << "\n";

    char* intwriter = new char[sizeof(int)];
    int* intbuffer = (int*) intwriter;

    char* vert = new char[sizeof(double) * 4];
    double* vert_buffer = (double*) vert;

    int min_x = -9; 
    int max_x = -9;
    int min_y = -9;
    int max_y = -9;
    int min_z = -9;
    int max_z = -9;
    //cout <<  "should be bad:" << min_x << " " << max_x << " " << min_y << " " << max_y << " " << min_z << " " << max_z << endl;


    intbuffer[0] = vertex.size();
    ofs.write(intwriter, sizeof(int));
    for (int i = 0; i < vertex.size(); i++){

    	int x = vertex[i].x;
    	int y = vertex[i].y;
    	int z = vertex[i].z;

    	//cout << min_z << endl;

    	if (z < -9)
    	{
    		cout << "bad z at vertex: " << i << endl;
    	}

    	if (min_x == -9 or x < min_x)
    		min_x = x;
    	if (max_x == -9 or x > max_x)
    		max_x = x;
    	if (min_y == -9 or y < min_y)
    		min_y = y;
    	if (max_y == -9 or y > max_y)
    		max_y = y;
    	if (min_z == -9 or z < min_z)
    		min_z = z;
    	if (max_z == -9 or z > max_z)
    		max_z = z;


        vert_buffer[0] = vertex[i].x; vert_buffer[1] = vertex[i].y;
        vert_buffer[2] = vertex[i].z; vert_buffer[3] = vertex[i].v;
        ofs.write(vert, sizeof(double) * 4);
    }

    //cout << "should be good: " << min_x << " " << max_x << " " << min_y << " " << max_y << " " << min_z << " " << max_z << endl;
    
    cout << "writing " << edge.size() << " edge\n";
    char* edgechar = new char[sizeof(int) * 2];
    int* edge_buffer = (int*) edgechar;
    intbuffer[0] = edge.size();
    ofs.write(intwriter, sizeof(int));
    for (int i = 0; i < edge.size(); i++){
        edge_buffer[0] = edge[i].p1; edge_buffer[1] = edge[i].p2;
        ofs.write(edgechar, sizeof(int) * 2);
    }
    
    cout << "writing " << triangle.size() << " triangle\n";
    char* trianglechar = new char[sizeof(int) * 3];
    int* triangle_buffer = (int*) trianglechar;
    intbuffer[0] = triangle.size();
    ofs.write(intwriter, sizeof(int));
    for (int i = 0; i < triangle.size(); i++){
        triangle_buffer[0] = triangle[i].p1;
        triangle_buffer[1] = triangle[i].p2;
        triangle_buffer[2] = triangle[i].p3;
        ofs.write(trianglechar, sizeof(int) * 3);
    }
    ofs.close();
}


int main(int argc, char* argv[])
{

	cout << "Lucas begin" << endl;
	string merged_dir;
	string input_dir;
	parameter para;
	vector<fileinfo> blocks;

	if (argc == 5){
		cout << "here" << endl;
		/*
		prefix = string(argv[1]);
		trans = string(argv[2]);
		setting_name = string(argv[3]);
		nb = 12;
		*/
		input_dir = argv[1];
		string output_dir(argv[2]);
		string configname(argv[3]);
		persistence_threshold = argv[4];
		cout << "Reading parameters from: " << 
			configname << endl;
		
		//merged_dir = output_dir + configname.substr(configname.find("merge-config") + 13, -4) + "/";
		int start = configname.find("merge-config") + 13;
		int end = configname.find(".");
		merged_dir = output_dir + string(&configname[start], &configname[end]) + "/";
		
		//return -1;
		init_file(configname, para, blocks);
		//return -1;
		//cout << para << endl;
		cout << "block count: " << blocks.size() << endl;
		
		//search_path = getpath(argv[1]);
		cout << "work folder: " << merged_dir << endl;
		//return 0;
	}
	else {
		cout << "usage: merge_graph <input_dir> <output_dir> <config_file> <persistence_threshold>";
		return 0;
	}

	adjust_bbox(blocks, para);

	
	vertex.clear(); 
	edge.clear(); 
	triangle.clear();
	
	norm_const = kernel_init(0.5);
	cout << "Normalize factor: " << norm_const << endl;
	
	//return -1;

	for(int i = 0; i < blocks.size(); i++){
		string filename = input_dir + blocks[i].name;
		blocks[i].name = filename;
		printf("Processing Graph %s...\n", filename.c_str());
		//cout << "\t" << vector<int>(blocks[i].offset, blocks[i].offset+ 6) << endl;
		
		ProcessGraph(blocks[i]);
	}

	Triangulate();

	simplex_output(merged_dir + "merge-complex");

	printf("Done\n");

	return 0;
}
