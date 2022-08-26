import os
import cv2
from math import ceil
import matplotlib.image as mpimg
import numpy as np
import csv
from multiprocessing import Pool
from functools import partial
import vtk

from PIL import Image
Image.MAX_IMAGE_PIXELS = None


def split_domain(input_dir, output_dir, x_len, y_len, z_len, overlap=5):
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

    valid_filename = os.path.join(output_dir, 'valid-dirs.txt')

    name_list = [name for name in os.listdir(input_dir) if
                 (os.path.isfile(input_dir + '/' + name)) and (name != ".DS_Store")]
    name_list.sort()

    # mask_list = [os.path.join(mask_dir, name) for name in os.listdir(mask_dir)]

    # assert(len(name_list) == len(mask_list))

    im = mpimg.imread(input_dir + name_list[0])
    nx, ny = im.shape
    nz = len(name_list)

    nx_cols = int(ceil(nx / x_len))
    ny_cols = int(ceil(ny / y_len))
    nz_cols = int(ceil(nz / z_len))

    number_of_tiles = nx_cols * ny_cols * nz_cols
    print('number of regions:', number_of_tiles, nx_cols, ny_cols, nz_cols)
    del im

    valid_cubes = []
    index = 0
    for k in range(nz_cols):
        z_min = max(k * z_len, 0)
        z_max = min((k + 1) * z_len + overlap, nz)

        image_list = []
        # mask_image_list = []
        for z_slice in range(z_min, z_max):
            name = name_list[z_slice]
            # mask = mask_list[z_slice]
            fileName = input_dir + "/" + name
            image_list.append(mpimg.imread(fileName))
            # mask_image_list.append(mpimg.imread(mask))

        for j in range(ny_cols):
            for i in range(nx_cols):
                # print('working on tile ', index, 'out of ', number_of_tiles)
                x_min = max(i * x_len, 0)
                x_max = min((i + 1) * x_len + overlap, nx)
                y_min = max(j * y_len, 0)
                y_max = min((j + 1) * y_len + overlap, ny)

                # print(x_min, y_min, z_min, x_max, y_max, z_max)

                cube = []
                for z_val in range(z_min, z_max):
                    cube.append(image_list[z_val - z_min][x_min:x_max, y_min:y_max])

                if np.max(cube) == 0:
                    print('skipping cube', index)
                    index += 1
                    continue

                tile_output_dir = output_dir + str(index) + '/'
                tile_image_output_dir = os.path.join(tile_output_dir, 'images/')
                if not os.path.exists(tile_output_dir):
                    os.mkdir(tile_output_dir)
                if not os.path.exists(tile_image_output_dir):
                    os.mkdir(tile_image_output_dir)
                coord_filename = os.path.join(tile_output_dir, 'coords.txt')
                with open(coord_filename, 'w') as coord_file:
                    coord_file.write(str(x_min) + ' ' + str(y_min) + ' ' + str(z_min) + '\n')
                    coord_file.close()
                slice_numbers = z_max - z_min
                digits = len(str(slice_numbers))
                for n in range(slice_numbers):
                    n_slice = cube[n]
                    '''
                    n_max = np.max(n_slice)
                    if n_max > slive_max:
                        slive_max = n_max
                    '''
                    # print('slice :', n_slice)
                    n_slice.astype('uint16')
                    # scipy.misc.toimage(n_slice, cmin=0.0, cmax=1.0).save(tile_output_dir + str(n).zfill(digits) + '.tif')
                    cv2.imwrite(tile_image_output_dir + str(n).zfill(digits) + '.tif', n_slice)
                # sys.exit()
                valid_cubes.append(index)
                index += 1

    with open(valid_filename, 'w') as valid_file:
        for v in valid_cubes:
            valid_file.write(str(v) + '\n')
        valid_file.close()
    
    return nx, ny, nz, overlap


def write_dipha_persistence_input(input_path):
    input_filename = os.path.join(input_path, 'valid-dirs.txt')
    os.system(
        "matlab -nosplash -nodisplay -nodesktop -r \'parallel_dipha_input(\"" + input_path + "\",\"" + input_filename + "\"); quit;\'")


def __single_dipha(directory):
    input_filename = 'dipha.input'
    diagram_filename = 'dipha.out'
    command = './DiMo3d/code/dipha-3d/build/dipha --upper_dim 2 ' + os.path.join(directory, input_filename) + ' ' + os.path.join(
        directory, diagram_filename) + ' ' + os.path.join(directory, 'dipha.edges')
    os.system(command)
    
    command = 'rm ' + os.path.join(directory, input_filename)
    os.system(command)
    command = 'rm ' + os.path.join(directory, diagram_filename)
    os.system(command)
    


def compute_dipha_persistence(input_path, threads=1):
    valid_dirs = []
    valid_dirs_filename = os.path.join(input_path, 'valid-dirs.txt')
    with open(valid_dirs_filename, 'r') as valid_dirs_file:
        reader = csv.reader(valid_dirs_file, delimiter=' ')
        for row in reader:
            valid_dirs.append(row[0])
        valid_dirs_file.close()
    dirs = [os.path.join(input_path, v) + '/' for v in valid_dirs]

    pool = Pool(threads)
    pool.map(__single_dipha, dirs)
    pool.close()
    pool.join()


def __single_convert(directory):
    dipha_edge_filename = os.path.join(directory, 'dipha.edges')
    output_filename = os.path.join(directory, 'dipha-edges.txt')
    os.system(
        "matlab -nosplash -nodisplay -nodesktop -r \'load_persistence_diagram(\"" + dipha_edge_filename + "\",\"" + output_filename + "\"); quit;\'")


def convert_persistence_diagram(input_path, threads=1):
    input_filename = os.path.join(input_path, 'valid-dirs.txt')
    cube_dirs = []
    with open(input_filename, 'r') as input_file:
        reader = csv.reader(input_file)
        for row in reader:
            cube_dirs.append(os.path.join(input_path, row[0]) + '/')
        input_file.close()

    # print(len(cube_dirs), 'dirs')

    # os.system('~/apps/MATLAB/R2021a/bin/matlab -nosplash -nodisplay -nodesktop -r')

    pool = Pool(threads)
    pool.map(__single_convert, cube_dirs)
    pool.close()
    pool.join()


def __single_write_vertex_files(directory):
    #print('working on', directory)
    vert_filename = os.path.join(directory, 'shifted-vert.txt')
    coord_filename = os.path.join(directory, 'coords.txt')
    image_dir = os.path.join(directory, 'images/')

    image_filenames = [os.path.join(image_dir, listing) for listing in os.listdir(image_dir)]
    image_filenames.sort()

    #print('images:', len(image_filenames))

    nz = len(image_filenames)
    image = mpimg.imread(image_filenames[0])
    nx, ny = image.shape
    del image

    #print(nx, ny, nz)
    # sys.exit()
    im_cube = np.zeros([nx, ny, nz])

    #print('reading images')
    i = 0
    for name in image_filenames:
        # print(i, name)
        im_cube[:, :, i] = mpimg.imread(name)
        i = i + 1

    with open(coord_filename, 'r') as coord_file:
        line = coord_file.readline()
        coord_file.close()
    shift = line.split(' ')
    sx = int(shift[0])
    sy = int(shift[1])
    sz = int(shift[2])

    # print('writing images')
    with open(vert_filename, 'w') as vert_file:
        for k in range(nz):
            # print('verts - working on image', k)
            for j in range(ny):
                for i in range(nx):
                    vert_file.write(str(i + sx) + ' ' + str(j + sy) + ' ' + str(k + sz) + ' ' + str(-im_cube[i, j, k]) + '\n')
        vert_file.close()


def write_vertex_files(input_path, threads=1):
    input_filename = os.path.join(input_path, 'valid-dirs.txt')
    cube_dirs = []
    with open(input_filename, 'r') as input_file:
        reader = csv.reader(input_file)
        for row in reader:
            cube_dirs.append(os.path.join(input_path, row[0]) + '/')
        input_file.close()

    pool = Pool(threads)
    pool.map(__single_write_vertex_files, cube_dirs)
    pool.close()
    pool.join()


def __run_morse_single(persistence_threshold, directory):
    threshold_dir = os.path.join(directory, str(persistence_threshold) + '/')
    if not os.path.exists(threshold_dir):
        os.mkdir(threshold_dir)
    vert_filename = os.path.join(directory, 'shifted-vert.txt')
    edge_filename = os.path.join(directory, 'dipha-edges.txt')
    command = './DiMo3d/code/dipha-output/a.out  ' + vert_filename + ' ' + edge_filename + ' ' + str(
        persistence_threshold) + ' ' + threshold_dir
    #  command = __PATH_TO_MORSE + '  ' + vert_filename + ' ' + edge_filename + ' ' + str(persistence_threshold) + ' ' + threshold_dir
    os.system(command)


def graph_reconstruction(input_path, persistence_threshold, threads=1):
    input_filename = os.path.join(input_path, 'valid-dirs.txt')
    # input_filename = os.path.join(input_path, __VALID_DIRS_FILENAME)
    cube_dirs = []
    with open(input_filename, 'r') as input_file:
        reader = csv.reader(input_file)
        for row in reader:
            cube_dirs.append(os.path.join(input_path, row[0]) + '/')
        input_file.close()

    pool = Pool(threads)
    pool.map(partial(__run_morse_single, persistence_threshold), cube_dirs)
    pool.close()
    pool.join()


def __write_merge_configs(input_dir, output_dir, nx, ny, nz, x_len, y_len, z_len, overlap):
    x_cubes = int(ceil(nx / x_len))
    y_cubes = int(ceil(ny / y_len))
    z_cubes = int(ceil(nz / z_len))

    # print('total cubes:', x_cubes * y_cubes * z_cubes)

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

    valid_dirs_filename = os.path.join(input_dir, 'valid-dirs.txt')
    merged_valid_dirs_filename = os.path.join(output_dir, 'valid-dirs.txt')

    # print('reading valid dirs')
    valid_dirs = set()
    with open(valid_dirs_filename, 'r') as valid_dirs_file:
        reader = csv.reader(valid_dirs_file, delimiter=' ')
        for row in reader:
            # print(int(row[0]))
            valid_dirs.add(int(row[0]))
            # print('added')
        valid_dirs_file.close()
    # print('valid dirs:', len(valid_dirs))
    # sys.exit()

    with open(merged_valid_dirs_filename, 'w') as merged_valid_dirs_file:
        count = 0
        for k in range(0, z_cubes, 2):
            for j in range(0, y_cubes, 2):
                for i in range(0, x_cubes, 2):
                    print('writing', count, 'th config file')
                    config_filename = os.path.join(output_dir, 'merge-config-' + str(count) + '.txt')

                    cube_list = []
                    start = k * x_cubes * y_cubes + j * y_cubes + i
                    # 1
                    cube_list.append([start, i, j, k, 1])
                    if i + 1 != x_cubes:
                        # 2
                        cube_list.append([start + 1, i + 1, j, k, 2])
                    if i + x_cubes < x_cubes * y_cubes:
                        # 3
                        cube_list.append([start + x_cubes, i, j + 1, k, 3])
                        if i + 1 != x_cubes:
                            # 4
                            cube_list.append([start + x_cubes + 1, i + 1, j + 1, k, 4])

                    if i + x_cubes * y_cubes < x_cubes * y_cubes * z_cubes:
                        # 5
                        cube_list.append([start + x_cubes * y_cubes, i, j, k + 1, 5])

                        if i + 1 != x_cubes:
                            # 6
                            cube_list.append([start + x_cubes * y_cubes + 1, i + 1, j, k + 1, 6])

                        if i + x_cubes < x_cubes * y_cubes:
                            # 7
                            cube_list.append([start + x_cubes * y_cubes + x_cubes, i, j + 1, k + 1, 7])
                            if i + 1 != x_cubes:
                                # 8
                                cube_list.append([start + x_cubes * y_cubes + x_cubes + 1, i + 1, j + 1, k + 1, 8])

                    valid = []

                    with open(config_filename, 'w') as config_file:
                        config_file.write('merge-complex\n')
                        config_file.write(str(overlap) + ' ' + str(overlap) + ' ' + str(overlap) + '\n')

                        for cube in cube_list:
                            s = cube[0]
                            if s not in valid_dirs:
                                continue
                            valid.append(s)
                            tag = cube[4]
                            if tag == 1:
                                sx = cube[1] * x_len - overlap
                                sy = cube[2] * y_len - overlap
                                sz = cube[3] * z_len - overlap
                                ex = sx + x_len + 2 * overlap - 1
                                ey = sy + y_len + 2 * overlap - 1
                                ez = sz + z_len + 2 * overlap - 1
                            elif tag == 2:
                                sx = cube[1] * x_len
                                sy = cube[2] * y_len - overlap
                                sz = cube[3] * z_len - overlap
                                ex = sx + x_len + 2 * overlap - 1
                                ey = sy + y_len + 2 * overlap - 1
                                ez = sz + z_len + 2 * overlap - 1
                            elif tag == 3:
                                sx = cube[1] * x_len - overlap
                                sy = cube[2] * y_len
                                sz = cube[3] * z_len - overlap
                                ex = sx + x_len + 2 * overlap - 1
                                ey = sy + y_len + 2 * overlap - 1
                                ez = sz + z_len + 2 * overlap - 1
                            elif tag == 4:
                                sx = cube[1] * x_len
                                sy = cube[2] * y_len
                                sz = cube[3] * z_len - overlap
                                ex = sx + x_len + 2 * overlap - 1
                                ey = sy + y_len + 2 * overlap - 1
                                ez = sz + z_len + 2 * overlap - 1
                            elif tag == 5:
                                sx = cube[1] * x_len - overlap
                                sy = cube[2] * y_len - overlap
                                sz = cube[3] * z_len
                                ex = sx + x_len + 2 * overlap - 1
                                ey = sy + y_len + 2 * overlap - 1
                                ez = sz + z_len + 2 * overlap - 1
                            elif tag == 6:
                                sx = cube[1] * x_len
                                sy = cube[2] * y_len - overlap
                                sz = cube[3] * z_len
                                ex = sx + x_len + 2 * overlap - 1
                                ey = sy + y_len + 2 * overlap - 1
                                ez = sz + z_len + 2 * overlap - 1

                            elif tag == 7:
                                sx = cube[1] * x_len - overlap
                                sy = cube[2] * y_len
                                sz = cube[3] * z_len
                                ex = sx + x_len + 2 * overlap - 1
                                ey = sy + y_len + 2 * overlap - 1
                                ez = sz + z_len + 2 * overlap - 1

                            else:
                                assert (tag == 8)
                                sx = cube[1] * x_len
                                sy = cube[2] * y_len
                                sz = cube[3] * z_len
                                ex = sx + x_len + 2 * overlap - 1
                                ey = sy + y_len + 2 * overlap - 1
                                ez = sz + z_len + 2 * overlap - 1

                            config_file.write(
                                str(s) + ' ' + str(sx) + ' ' + str(sy) + ' ' + str(sz) + ' ' + str(ex) + ' ' + str(
                                    ey) + ' ' + str(ez) + '\n')
                        config_file.close()

                    if len(valid) > 0:
                        merged_valid_dirs_file.write(str(count) + '\n')

                    count += 1
    merged_valid_dirs_file.close()


def __single_merge(raw_dir, merge_dir, persistence_threshold, merge_threshold, d):
    output_dir = os.path.join(merge_dir, str(d) + '/')
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)

    complex_filename = os.path.join(output_dir, 'merge-complex.sc')
    # if not os.path.exists(complex_filename):
    config_filename = os.path.join(merge_dir, 'merge-config-' + str(d) + '.txt')
    merge_command = './DiMo3d/code/merge/a.out ' + raw_dir + ' ' + merge_dir + ' ' + config_filename + ' ' + str(persistence_threshold)
    print(merge_command)
    os.system(merge_command)

    #return

    morse_dir = os.path.join(output_dir, str(persistence_threshold) + '/')
    if not os.path.exists(morse_dir):
        os.mkdir(morse_dir)
    morse_command = './DiMo3d/code/spt_cpp/spt_cpp' + ' ' + complex_filename + ' ' + morse_dir + ' ' + str(merge_threshold) + ' 3'
    os.system(morse_command)


def __execute_merging(input_dir, output_dir, persistence_threshold, merge_threshold, threads=1):

    merged_valid_dirs_filename = os.path.join(output_dir, 'valid-dirs.txt')

    valid = []
    with open(merged_valid_dirs_filename, 'r') as mvdf:
        reader = csv.reader(mvdf, delimiter=' ')
        for row in reader:
            valid.append(int(row[0]))
        mvdf.close()

    pool = Pool(threads)
    pool.map(partial(__single_merge, input_dir, output_dir, persistence_threshold, merge_threshold), valid)
    pool.close()
    pool.join()


def merge(input_path, merge_dir, persistence_threshold, merge_threshold, nx, ny, nz, x_len, y_len, z_len, overlap, threads=1):

    if not os.path.exists(merge_dir):
        os.mkdir(merge_dir)

    current_dir = input_path
    count = 0
    next_output_dir = os.path.join(merge_dir, str(count)) + '/'

    while len([listing for listing in os.listdir(current_dir) if os.path.isdir(os.path.join(current_dir, listing))]) > 1:

        print('merge count:', count)
        if not os.path.exists(next_output_dir):
            os.mkdir(next_output_dir)

        __write_merge_configs(current_dir, next_output_dir, nx, ny, nz, x_len, y_len, z_len, overlap)

        __execute_merging(current_dir, next_output_dir, persistence_threshold, merge_threshold, threads=threads)

        # update variables for next round of merging
        x_len *= 2
        y_len *= 2
        z_len *= 2
        count += 1
        current_dir = next_output_dir
        next_output_dir = os.path.join(merge_dir, str(count)) + '/'
        print([listing for listing in os.listdir(current_dir) if os.path.isdir(os.path.join(current_dir, listing))])

