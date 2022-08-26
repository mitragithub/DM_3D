parentdir = '/nfs/data/main/M33/lucas/fmost/191196-split-v2/';
fid = fopen('valid-dirs.txt');


count = 1;
while ~feof(fid)
    x(count).name = fgetl(fid);
    count = count + 1;
end
fclose(fid);

parpool('local', 52);
parfor i = 1 : length(x)
    image_folder_path = [parentdir x(i).name '/'];
    dipha_input_filename = [image_folder_path 'dipha-v4.input'];
    save_3d_image_data( image_folder_path, dipha_input_filename );
end
