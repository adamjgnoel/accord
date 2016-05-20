function [hFig, hAxes] = accordPlotEnvironment(config, axesProp, figureProp, ...
    regionDispStruct, actorDispStruct, scale)
%
% The AcCoRD Simulator
% (Actor-based Communication via Reaction-Diffusion)
%
% Copyright 2016 Adam Noel. All rights reserved.
% 
% For license details, read LICENSE.txt in the root AcCoRD directory
% For user documentation, read README.txt in the root AcCoRD directory
%
% accordPlotEnvironment.m - plot AcCoRD simulation environment based on the
%   environment configuration
%
% INPUTS
% config - structure of simulation configuration settings. Created by
%   accordImport in call to accordConfigImport
% axesProp - structure with common axes properties. Created by call to
%   accordBuildAxesStruct
% figureProp - structure with common figure properties. Created by call to
%   accordBuildFigureStruct
% regionDispStruct - structure describing what regions to display and how
%   to display them. Structure initialized by accordBuildDispStruct. Can be
%   passed as an empty variable, and then all regions will be plotted with
%   default parameters
% actorDispStruct - structure describing what actors to display and how
%   to display them. Structure initialized by accordBuildDispStruct. Can be
%   passed as an empty variable, and then all actors will be plotted with
%   default parameters
% scale - scaling of physical dimensions of region and actor coordinates.
%   Needed to mitigate patch display problems. Recommend that smallest
%   object to plot has dimension of order 1
%
% OUTPUTS
% hFig - handle to plotted figure. Use for making changes to figure.
% hAxes - handle to axes in plotted figure. Use for making changes to
%   axes.
%
% Last revised for AcCoRD LATEST_VERSION
%
% Revision history:
%
% Revision LATEST_VERSION
% - Created file
%
% Created 2016-05-17

%% Create figure and apply properties
hFig = figure;
if ~isempty(figureProp)
    figurePropFields = fieldnames(figureProp);
    numProp = numel(figurePropFields);
    for i = 1:numProp
        set(hFig, figurePropFields{i}, figureProp.(figurePropFields{i}));
    end
end

%% Create axes and apply properties
hAxes = axes('Parent',hFig);
axis equal
if ~isempty(axesProp)
    axesPropFields = fieldnames(axesProp);
    numProp = numel(axesPropFields);
    for i = 1:numProp
        set(hAxes, axesPropFields{i}, axesProp.(axesPropFields{i}));
    end
end

hold('on');

% Extract region display information
if isempty(regionDispStruct)
    % Display all regions with default settings
    regionDispStruct = accordBuildDispStruct(1:config.numRegion);
    regionDispStruct.dispColor{1} = [];
end
if isempty(regionDispStruct.dispColor{1})
    % Colors not defined. Use default colormap values
    colormapRegion = parula(regionDispStruct.numToDisp);
    regionDispStruct.dispColor{1} = cell(1,regionDispStruct.numToDisp);
    for i = 1:regionDispStruct.numToDisp
        regionDispStruct.dispColor{1}{i} = colormapRegion(i,:);
    end
end

% Plot the regions
regionHandles = zeros(1,regionDispStruct.numToDisp);
for i = 1:regionDispStruct.numToDisp
    
    [scaleDim, plane] = accordCalculateRegionPlotParam(...
        config.region{regionDispStruct.indToDisp(i)}, config);
    
    dispStr = sprintf('Region %d',regionDispStruct.indToDisp(i)-1);
    
    regionHandles(i) = accordPlotShape(...
        config.region{regionDispStruct.indToDisp(i)}.shape, scale, ...
        scaleDim, plane, ...
        config.region{regionDispStruct.indToDisp(i)}.anchorCoor, ...
        dispStr, regionDispStruct.bDispFace(i), ...
        regionDispStruct.opaque(i), regionDispStruct.dispColor{1}{i});
end

% Extract actor display information
if isempty(actorDispStruct)
    % Display all actors with default settings
    actorDispStruct = accordBuildDispStruct(1:config.numActor);
    actorDispStruct.dispColor{1} = [];
end
if isempty(actorDispStruct.dispColor{1})
    % Colors not defined. Use default colormap values
    colormapActor = jet(actorDispStruct.numToDisp);
    actorDispStruct.dispColor{1} = cell(1,actorDispStruct.numToDisp);
    for i = 1:actorDispStruct.numToDisp
        actorDispStruct.dispColor{1}{i} = colormapActor(i,:);
    end
end

% Plot the actors
actorHandles = cell(1,actorDispStruct.numToDisp);
for i = 1:actorDispStruct.numToDisp
    curActor = actorDispStruct.indToDisp(i);
    dispStr = sprintf('Actor %d',curActor-1);
    if config.actor{curActor}.bDefinedByRegions
        actorHandles{i} = zeros(1,config.actor{curActor}.numRegion);
        for j = 1:config.actor{curActor}.numRegion
            regionInd = findRegion(config, ...
                config.actor{curActor}.regionList{j});
            if regionInd > 0
                [scaleDim, plane] = ...
                    accordCalculateRegionPlotParam(config.region{regionInd}, config);
            end
            
            actorHandles{i}(j) = accordPlotShape(config.region{regionInd}.shape, scale, ...
                scaleDim, plane, config.region{regionInd}.anchorCoor, ...
                dispStr, actorDispStruct.bDispFace(i), ...
                actorDispStruct.opaque(i), actorDispStruct.dispColor{1}{i});
        end
    else
        [scaleDim, plane, anchorCoor] = accordActorPlotParam(config.actor{curActor});
        actorHandles{i} = accordPlotShape(config.actor{curActor}.shape, scale, ...
            scaleDim, plane, anchorCoor, ...
            dispStr, actorDispStruct.bDispFace(i), ...
            actorDispStruct.opaque(i), actorDispStruct.dispColor{1}{i});
    end
    
    
end

end

% Find index of region with matching label
function regionInd = findRegion(config, regionStr)
    for i = 1:length(config.region)
        if strcmp(config.region{i}.label, regionStr)
            regionInd = i;
            return
        end
    end
    warning('Region with label %s could not be found', regionStr);
    regionInd = 0;
    return;
end

% Return parameters needed to plot actor shape
function [scaleDim, plane, anchorCoor] = accordActorPlotParam(actor)
    plane = 0;
    if strcmp(actor.shape, 'Rectangular Box') || ...
        strcmp(actor.shape, 'Rectangle')
        % Scale vertices by size
        scaleDim = [actor.boundary(2) - actor.boundary(1), ...
            actor.boundary(4) - actor.boundary(3), ...
            actor.boundary(6) - actor.boundary(5)];
        anchorCoor = [actor.boundary(1) actor.boundary(3) actor.boundary(5)];
        if strcmp(actor.shape, 'Rectangle')
            if actor.boundary(1) == actor.boundary(2)
                plane = 1;
            elseif actor.boundary(3) == actor.boundary(4)
                plane = 2;
            elseif actor.boundary(5) == actor.boundary(6)
                plane = 3;
            end
        end
    elseif strcmp(actor.shape, 'Sphere')
        anchorCoor = [actor.boundary(1) actor.boundary(2) actor.boundary(3)];
        scaleDim = actor.boundary(4);
    end
end

% Return parameters needed to plot region shape
function [scaleDim, plane] = accordCalculateRegionPlotParam(region, config)
    plane = 0;
    if strcmp(region.shape, 'Rectangular Box') || ...
        strcmp(region.shape, 'Rectangle')
        % Scale vertices by size
        scaleDim = [region.numSubDim(1)*...
            region.subvolSizeInt*config.subBaseSize, ...
            region.numSubDim(2)*...
            region.subvolSizeInt*config.subBaseSize, ...
            region.numSubDim(3)*...
            region.subvolSizeInt*config.subBaseSize];
        if strcmp(region.shape, 'Rectangle')
            if region.numSubDim(1) == 0
                plane = 1;
            elseif region.numSubDim(2) == 0
                plane = 2;
            elseif region.numSubDim(3) == 0
                plane = 3;
            end
        end
    elseif strcmp(region.shape, 'Sphere')
        scaleDim = region.radius;
    end
end

% Plot one shape (Could be region or part of an actor)
function h = accordPlotShape(shape, scale, scaleDim, plane, moveDim, ...
    dispStr, bSurf, opaque, plotColor)
    % Box structure
    faces = [1 2 6 5; 2 3 7 6; 3 4 8 7; 4 1 5 8; 1 2 3 4; 5 6 7 8];
    vertices = [0 0 0; 1 0 0; 1 1 0; 0 1 0; 0 0 1; 1 0 1; 1 1 1; 0 1 1];
    
    if strcmp(shape, 'Rectangular Box')
        % Scale vertices by size
        curVertices = vertices;
        curFaces = faces;
        curVertices([2 3 6 7],1) = ...
            vertices([2 3 6 7],1)*scaleDim(1);
        curVertices([3 4 7 8],2) = ...
            vertices([3 4 7 8],2)*scaleDim(2);
        curVertices([5 6 7 8],3) = ...
            vertices([5 6 7 8],3)*scaleDim(3);
    elseif strcmp(shape, 'Rectangle')
        % Need to determine which axis rectanle lies in
        curFaces = [1 2 3 4];
        if plane == 1
            curVertices = vertices([1 4 8 5],:);
            curVertices([2 3],2) = ...
                vertices([4 8],2)*scaleDim(2);
            curVertices([4 3],3) = ...
                vertices([5 8],3)*scaleDim(3);
        elseif plane == 2
            curVertices = vertices([1 2 6 5],:);
            curVertices([2 3],1) = ...
                vertices([2 6],1)*scaleDim(1);
            curVertices([4 3],3) = ...
                vertices([5 6],3)*scaleDim(3);
        elseif plane == 3
            curVertices = vertices([1 2 3 4],:);
            curVertices([2 3],1) = ...
                vertices([2 3],1)*scaleDim(1);
            curVertices([3 4],2) = ...
                vertices([3 4],2)*scaleDim(2);
        end
    elseif strcmp(shape, 'Sphere')
        % Easier to generate sphere points in "surface" mode and then
        % convert to patch for consistency
        [X, Y, Z] = sphere(15);
        X = scaleDim*X;
        Y = scaleDim*Y;
        Z = scaleDim*Z;
        [curFaces, curVertices, ~] = surf2patch(X,Y,Z);
    else
        warning('Shape %s not recognized\n',shape);
        h = 0;
        return;
    end
    
    for j = 1:3
        curVertices(:,j) = ...
            scale*(curVertices(:,j) + moveDim(j));
    end
    
    if bSurf
        h = patch('Vertices',curVertices,'Faces',curFaces, ...
            'FaceColor', plotColor, 'FaceAlpha', opaque, ...
            'DisplayName', dispStr);
    else
        h = patch('Vertices',curVertices,'Faces',curFaces, 'FaceColor', 'none', ...
            'EdgeColor', plotColor, 'FaceAlpha', opaque, ...
            'DisplayName', dispStr);
    end
end