import DiMo3d as dm
import sys


def test_3d_func():
    input_dir = sys.argv[1]
    output_dir = sys.argv[2]
    merge_dir = sys.argv[3]

    threads = 1

    # split domain into overlapping cubes
    #nx, ny, nz, overlap = dm.split_domain(input_dir, output_dir, 128, 128, 128, 16)

    # prepare dipha files for each individual cube to compute persistence
    #dm.write_dipha_persistence_input(output_dir)

    # run dipha to compute persistence
    # dm.compute_dipha_persistence(output_dir, threads)

    # convert dipha outputs to usable format
    #dm.convert_persistence_diagram(output_dir, threads)

    # write vertices
    #dm.write_vertex_files(output_dir, threads)

    # run morse
    #dm.graph_reconstruction(output_dir, 256, threads)

    # merge
    # merge(input_path, merge_dir, persistence_threshold, merge_threshold, nx, ny, nz, x_len, y_len, z_len, overlap, threads=1)
    dm.merge(output_dir, merge_dir, 256, 256, 256, 256, 256, 128, 128, 128, 16, threads=1)


if __name__ == '__main__':
    test_3d_func()
