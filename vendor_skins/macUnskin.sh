# Copyright Dr. Alex. Gouaillard (2015, 2020)

# list all files with .old extention
# for each file in the list
#   if the file is the same as its pendent in /vendor_skin/<vendor>
#     delete
#   else
#     copy back in vendor_skin
#
git checkout -- CI/*
rm UI/webrtcVersion.h
git checkout -- UI/*
git checkout -- cmake/*

