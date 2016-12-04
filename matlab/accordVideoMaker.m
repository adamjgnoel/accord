function [hFig, hAxes] = accordVideoMaker(fileToLoad,...
    bMakeVideo, videoName, videoFormat, ...
    curRepeat, scale, observationToPlot, ...
    customFigProp, customAxesProp, customVideoProp, ...
    regionToPlot, customRegionProp, actorToPlot, customActorProp,...
    passiveActorToPlot, molToPlot, customMolProp, ...
    cameraAnchorArray, frameCameraAnchor)
%
% The AcCoRD Simulator
% (Actor-based Communication via Reaction-Diffusion)
%
% Copyright 2016 Adam Noel. All rights reserved.
% 
% For license details, read LICENSE.txt in the root AcCoRD directory
% For user documentation, read README.txt in the root AcCoRD directory
%
% accordVideoMaker.m - Generate a video file from AcCoRD simulation output.
%   Or, generate a sequence of images frp, AcCoRD simulation output
%   Due to the large number of complex input arguments, this function
%   should as specified by the accordVideoMarkerWrapper function or a
%   similarly-structured function or script. Input arguments specify what
%   components of the simulation environment to draw and how, including the
%   regions, actors, and molecules. Inputs also specify how to display the
%   plotting figure and make the video. After the initial plotting of the
%   environment, the function passes temporary control to the user so that
%   the figure can be tweaked manually if desired.
%
% INPUTS
% fileToLoad - simulation file generated by accordImport
% bMakeVideo - if true, plot all simulation in a single figure and stich
%   images into a video file. If false, plot each observation in its own
%   figure and include an empty initial figure
% videoName - filename to save the video to
% videoFormat - file format of the video
% curRepeat - index of the simulation realization to plot
% scale - scaling of physical dimensions of region and actor coordinates.
%   Needed to mitigate patch display problems. Recommend that smallest
%   object (non-molecule) to plot has dimension of order 1
% observationToPlot - array of indices of observations to plot. Assume that
%   all actors being displayed make the same number of observations and at
%   the same times
% customFigProp - structure of figure properties to change from AcCoRD
%   defaults. Can be passed as empty if no defaults are to be changed. See
%   accordBuildFigureStruct for structure fields and their default values.
% customAxesProp - structure of axes properties to change from AcCoRD
%   defaults. Can be passed as empty if no defaults are to be changed. See
%   accordBuildAxesStruct for structure fields and their default values.
% customVideoProp - structure of video properties to change from AcCoRD
%   defaults. Can be passed as empty if no defaults are to be changed. See
%   accordInitializeVideo for structure fields and their default values.
% regionToPlot - array of indices of regions to be plotted.
% customRegionProp - structure of region properties to change from AcCoRD
%   defaults. Can be passed as empty if no defaults are to be changed. See
%   accordBuildDispStruct for structure fields and their default values.
% actorToPlot - array of indices of actors to be plotted. Indexing matches
%   the actor list in the original config file and is independent of
%   whether an actor is active or passive. Actors listed here will have
%   their shapes plotted but NOT their molecules (the latter is indicated
%   by the argument "passiveActorToPlot". Actors are drawn after regions,
%   so if an actor is defined by region(s) then it will be drawn on top of
%   its region(s).
% customActorProp - structure of actor properties to change from AcCoRD
%   defaults. Can be passed as empty if no defaults are to be changed. See
%   accordBuildDispStruct for structure fields and their default values.
% passiveActorToPlot - array of indices of passive recording actors whose
%   molecules are to be plotted. Indexing matches the list of passive
%   actors whose observations are recorded (as stored in the data structure
%   in fileToLoad). Actors listed here will NOT have their shapes plotted
%   (use actorToPlot to plot specific actors).
% molToPlot - cell array of arrays of indices of molecules to display. Each
%   cell corresponds to the passive actor specified in passiveActorToPlot.
%   Its array lists the indices of the molecule types of that actor that
%   are to be drawn. The indexing of the actor's array corresponds to the
%   list of molecules that the actor is recording (as stored in the data
%   structure in fileToLoad).
% customMolProp - cell array of structure of molecule properties to change
%   from AcCoRD defaults. Can be passed as empty if no defaults are to be
%   changed. Indexing in this cell array matches that of the molToPlot cell
%   array. See accordBuildMarkerStruct for structure fields and their
%   default values.
% cameraAnchorArray - cell array of cell arrays defining anchor points for
%   the camera display. Can be passed as an empty cell array. Each anchor
%   point is a cell array defining a complete set of camera settings, in
%   the format {'CameraPosition', 'CameraTarget', 'CameraViewAngle',
%   'CameraUpVector'}. See MATLAB camera documentation for more details.
% frameCameraAnchor - array that specifies which frames use which camera
%   anchors. Length of array should be equal to length of
%   observationToPlot. If cameraAnchorArray is empty, then this array can
%   also be empty. Values in array must match indices of anchor points in
%   cameraAnchorArray. If a frame has associated value 0, and there are
%   camera anchors defined, then the camera settings will be interpolated
%   between anchors.
%
% OUTPUTS
% hFig - handle(s) to plotted figure(s). Use for making changes.
% hAxes - handle(s) to axes in plotted figure(s). Use for making changes.
%
% Last revised for AcCoRD v1.0 (2016-10-31)
%
% Revision history:
%
% Revision v1.0 (2016-10-31)
% - expanded the bMakeVideo bool to also be an int to specify how many
% times to repeat each frame. Use to reduce the apparent frame rate
% - added option to display a molecule counter for any specified passive
% actor and recorded molecule type. Facilitated by subfunction
% accordAddObservationCount(PASSIVE_ACTOR_INDEX,MOLECULE_INDEX,TEXT).
% Counter also includes custom prefix text.
% - changed default annotation background color to white (previously
% transparent)
%
% Revision v0.6 (public beta, 2016-05-30)
% - Created file
%
% Created 2016-05-19

%% Load Simulation Output
load(fileToLoad, 'config', 'data');

%% Confirm Size of Molecule Arrays
numFrames = length(observationToPlot);
numPassiveObservers = length(passiveActorToPlot);
numMolPropStruct = length(customMolProp);
numMolToPlotArray = length(molToPlot);
if ~isempty(customMolProp) && numMolPropStruct ~= numPassiveObservers
    warning('Incorrect number of custom molecule property structures defined.\n');
    warning('Structure array should have same length as number of passive actors whose observations are being recorded.\n');
    warning('Copying first structure in customMolProp.\n');
    customMolProp = repmat(customMolProp(1),1,numPassiveObservers);
end
if numMolToPlotArray ~= numPassiveObservers
    warning('Incorrect number of molecule plotting arrays defined.\n');
    warning('Cell array should have same length as number of passive actors whose observations are being recorded.\n');
    warning('Copying first array in molToPlot.\n');
    molToPlot = repmat(molToPlot{1},1,numPassiveObservers);
end

%% Load Default Display Properties and Apply Specified Changes
figureProp = accordBuildFigureStruct(customFigProp);
axesProp = accordBuildAxesStruct(customAxesProp);
regionDispStruct = accordBuildDispStruct(regionToPlot, customRegionProp);
actorDispStruct = accordBuildDispStruct(actorToPlot, customActorProp);
molStructArray = accordBuildMarkerStruct(numPassiveObservers, molToPlot, customMolProp);

if bMakeVideo
    %% Plot Static Environment (Video Background)
    [hFig, hAxes] = accordPlotEnvironment(config, axesProp, figureProp, ...
        regionDispStruct, actorDispStruct, scale);

    %% Create and Open Video Object
    videoObj = accordInitializeVideo(videoName, videoFormat, customVideoProp);
    
    %% Check repeat factor
    if ~isa(bMakeVideo, 'logical')
        repeatFactor = bMakeVideo;
    else
        repeatFactor = 1;
    end
else
    %% Generating Series of Figures Instead of a Video. Plot Empty Environment
    hFig = gobjects(1,1+numFrames);
    hAxes = gobjects(1,1+numFrames);
    [curFig, curAxes] = accordPlotEnvironment(config, axesProp, figureProp, ...
        regionDispStruct, actorDispStruct, scale);
    hFig(1) = curFig;
    hAxes(1) = curAxes;  
end

numObs = length(data.passiveRecordCount{passiveActorToPlot(1)}(curRepeat,molToPlot{1},:));

%% Allow User To Adjust Environment View and Set Dynamic Behavior
disp('Adjust figure camera angles or annotations if desired.');

bDispTime = false;
numCounter = 0;
numPt = 0;
disp('Call accordAddTimeDisplay(NUM_DECIMAL_PLACES) to print dynamic simulation time in seconds,');
disp(' where NUM_DECIMAL_PLACES is the number of decimal places.');
disp(' The default is 4 places.');

disp('Call accordAddObservationCount(PASSIVE_ACTOR_INDEX,MOLECULE_INDEX,TEXT) to print a molecule counter.');
disp(' PASSIVE_ACTOR_INDEX is the index of passive actor to count, from recorded passive actor list.');
disp(' MOLECULE_INDEX is the index of molecule to count, from actor''s molecule list.');
disp(' TEXT is the prefix text that will appear before the counter value.');
disp(' Counter will only display a number and no text.');

disp('Call accordAddCameraAnchor(FRAME_INDICES) to anchor the camera display settings,');
disp('	for the video frames defined by FRAME_INDICES.');
disp('	For frames that are not defined, the camera display is interpolated between');
disp('	anchor frames.');
disp('	If accordAddCameraAnchor is called without arguments, then existing camera');
disp('	anchor settings are reset and all frames are anchored to the current.');
disp('	camera.');
disp('	Current settings can be checked by reading variables');
disp('	cameraAnchorArray and frameCameraAnchor.');
disp('  Call accordViewCameraAnchor(ANCHOR_IND) to apply the camera settings');
disp('  defined for anchor ANCHOR_IND.');

numCameraAnchor = length(cameraAnchorArray);
bDynamicCamera = numCameraAnchor > 0;
if bDynamicCamera
    disp('NOTE: Camera anchor points already defined by wrapper file.');
    disp('	Calls to accordAddCameraAnchor will append to current settings.');
    disp('	Camera is set to first camera anchor.');
    set(hAxes(1), {'CameraPosition','CameraTarget',...
        'CameraViewAngle','CameraUpVector'}, cameraAnchorArray{1});
else
    disp('NOTE: No camera anchor points already defined by wrapper file.');
    disp('	If accordAddCameraAnchor is not called, then camera settings');
    disp('	will not be changed when video generation starts.');
    frameCameraAnchor = zeros(1,numFrames);
end

disp('Call accordAddAnnotation(FRAME_INDICES) to save the current figure annotations,');
disp('	to display for the video frames defined by FRAME_INDICES.');
disp('	Subsequent calls to accordAddAnnotation with the same indices will');
disp('	override the previous annotations for those frames.');
disp('	The timer display is treated as a separate annotation and is not affected');
disp('	by calls to accordAddAnnotation.');
disp('	Any annotations remaining on the figure at the start of the video');
disp('	will remain for the entire video.');
hInvisible = figure('Visible', 'off');
annotation(hInvisible, 'textbox', [0.01 0.01 0.01 0.01], ...
    'String', '');
hAnnotInvisAxes = findall(hInvisible, 'Tag', 'scribeOverlay');
bDynamicAnnotation = false;
numAnnotatedFrames = 0;
frameAnnotation = zeros(1, numFrames);

disp('Type dbcont to continue or dbquit to quit');
% Check with user if number of frames is large
if ~bMakeVideo && numFrames > 10
    warning('Configuration will create %d figures instead of a video, which is a large number of new figure windows', numFrames);
end
keyboard

%% Lock in axis limits and store inital camera view
figureProp.Position = get(hFig(1),'Position');
curAxis = axis(hAxes(1));
initCamera = get(hAxes(1), {'CameraPosition','CameraTarget',...
    'CameraViewAngle','CameraUpVector'});
set(hAxes(1),{'CameraPositionMode','CameraTargetMode',...
    'CameraViewAngleMode','CameraUpVectorMode'}, ...
    {'manual', 'manual', 'manual', 'manual'});
set(hAxes(1), {'CameraPosition','CameraTarget',...
    'CameraViewAngle','CameraUpVector'}, initCamera);

if bDynamicCamera
    firstCameraAnchor = find(frameCameraAnchor,1);
    lastCameraAnchor = find(frameCameraAnchor,1, 'last');
end

if bDynamicAnnotation || ~bMakeVideo
    hAnnotInvis = findall(hInvisible, 'Tag', 'scribeOverlay');
    if ~bMakeVideo
        % Are there any static annotations? If so then store them
        hAnnotAxes = findall(hFig(1), 'Tag', 'scribeOverlay');
        hAnnot = get(hAnnotAxes, 'Children');
        if bDispTime
            if iscell(hAnnot)
                hAnnot = [hAnnot{:}];
            end
            % Find and exclude time display
            for hCur = 1:length(hAnnot)
                if strcmp('Timer Box', get(hAnnot(hCur),'Tag'))
                    hAnnot(hCur) = [];
                    break
                end
            end
        end
        if numCounter > 0            
            if iscell(hAnnot)
                hAnnot = [hAnnot{:}];
            end
            % Find and exclude counter displays
            for curCounter = 1:numCounter
                for hCur = 1:length(hAnnot)
                    if strcmp(['Counter ' num2str(numCounter)], get(hAnnot(hCur),'Tag'))
                        hAnnot(hCur) = [];
                        break
                    end
                end
            end
        end
        set(hAnnot, 'Tag', 'Static Annotation');
        copyobj(hAnnot, hAnnotInvisAxes);
    end
end

if bDispTime
    hDispTimeCopy = copyobj(hTimer,hAnnotInvisAxes);
    hTimerArray = zeros(1,numFrames);
end
if numCounter > 0
    hDispCounterCopy = zeros(1,numCounter);
    for i = 1:numCounter
        hDispCounterCopy(i) = copyobj(hObsCount(i),hAnnotInvisAxes);
    end
    hCounterArray = zeros(numCounter,numFrames);
end

%% Make the Movie
% Assume that number of observations of first actor is same as number for
% all other passive actors being recorded
hPoints = cell(1,numPassiveObservers);
for i = 1:numPassiveObservers
    hPoints{i} = length(molToPlot{i});
end

for i = 1:numFrames
    if observationToPlot(i) > numObs
        % Assume that frame indexing is indexing and we have reached the
        % end of the simulation
        break
    else
        if bMakeVideo
            curAxes = hAxes;
            curFig = hFig;
        else
            [curFig, curAxes] = accordPlotEnvironment(config, axesProp, figureProp, ...
                regionDispStruct, actorDispStruct, scale);
            hAxes(i+1) = curAxes;
            hFig(i+1) = curFig;
            % Give new figure an invisible string annotation so that we can
            % copy new annotations to it if needed
            annotation(curFig, 'textbox', [0.01 0.01 0.01 0.01], ...
                'String', '', 'Visible', 'off');
        end
        hCurAnnotAxes = findall(curFig, 'Tag', 'scribeOverlay');
        % Plot all of the specified molecules
        for j = 1:numPassiveObservers
            hPoints{j} = accordPlotSingleObservation(curAxes, data, ...
                passiveActorToPlot(j), molStructArray{j}, ...
                observationToPlot(i), curRepeat, scale);
        end
        axis(curAxes, curAxis);
        if bDynamicCamera
            % Determine what camera values to use
            if frameCameraAnchor(i) > 0
                % Camera view explicitly specified
                curCamera = cameraAnchorArray{frameCameraAnchor(i)};
            elseif i < firstCameraAnchor
                % Use first camera anchor
                curCamera = cameraAnchorArray{frameCameraAnchor(firstCameraAnchor)};
            elseif i > lastCameraAnchor
                % Use last camera anchor
                curCamera = cameraAnchorArray{frameCameraAnchor(lastCameraAnchor)};
            else
                % Need to interpolate camera between anchors
                anchor1Ind = find(frameCameraAnchor(1:i),1,'last');
                anchor2Ind = i+find(frameCameraAnchor((i+1):end),1);
                anchor1 = cameraAnchorArray{frameCameraAnchor(anchor1Ind)};
                anchor2 = cameraAnchorArray{frameCameraAnchor(anchor2Ind)};
                prog = (i - anchor1Ind)/(anchor2Ind - anchor1Ind);
                for j = 1:4
                    curCamera{j} = anchor1{j} + prog*(anchor2{j} - anchor1{j});
                end
            end
            set(curAxes, {'CameraPosition','CameraTarget',...
                'CameraViewAngle','CameraUpVector'}, curCamera);
        else
            set(curAxes, {'CameraPosition','CameraTarget',...
                'CameraViewAngle','CameraUpVector'}, initCamera);
        end
        if bDynamicAnnotation && frameAnnotation(i) > 0
            hCurAnnotInvis = findall(hAnnotInvis, 'Tag', num2str(frameAnnotation(i)));
            hCurAnnot = copyobj(hCurAnnotInvis, hCurAnnotAxes);
        end
        if bDispTime
            if bMakeVideo
                set(hTimer, 'String', sprintf('%.*f s',numPt, tArray(i)));
            else
                hTimerArray(i) = copyobj(hDispTimeCopy,hCurAnnotAxes);
                set(hTimerArray(i), 'String', sprintf('%.*f s',numPt, tArray(i)));
            end
        end
        for curCounter = 1:numCounter
            if bMakeVideo
                set(hObsCount(curCounter), 'String', [counterText{curCounter} num2str(counterArray{curCounter}(i))]);
            else
                hCounterArray(curCounter,i) = copyobj(hDispCounterCopy(curCounter),hCurAnnotAxes);
                set(hCounterArray(curCounter), 'String', [counterText{curCounter} num2str(counterArray{curCounter}(i))]);
            end
        end
        if bMakeVideo
            % Capture frame and add to video
            drawnow
            for r = 1:repeatFactor
                frame = getframe(curFig);
                writeVideo(videoObj,frame);
            end
            % Remove plots of molecules
            for j = 1:numPassiveObservers
                delete(hPoints{j});
            end
            if bDynamicAnnotation && frameAnnotation(i) > 0
                % Remove annotations
                delete(hCurAnnot);
            end
        else
            % Add static annotations if there were any
            hCurAnnotInvis = findall(hAnnotInvis, 'Tag', 'Static Annotation');
            hCurAnnot = copyobj(hCurAnnotInvis, hCurAnnotAxes);
        end
    end
end

%% Cleanup
delete(hInvisible);
if bMakeVideo
    close(videoObj)
    if bDispTime
        delete(hTimer);
    end
    annotation(hFig, 'textbox', [0.01 0.01 0.1 0.1], ...
        'String', 'Video Completed!');
end



    %% Nested Functions That User Can Call When Adjusting View
    
    % Add timer display. Apply same text properties as those used in axes
    % properties structure
    function accordAddTimeDisplay(numPtUser)
        bDispTime = true;
        hTimer = annotation(hFig(1), ...
            'textbox', [0.01 0.01 0.1 0.1], ...
            'Tag', 'Timer Box', ...
            'String', 'Timer String', ...
            'FontName', axesProp.FontName, ...
            'FontSize', axesProp.FontSize, ...
            'FontWeight', axesProp.FontWeight, ...            
            'Interpreter', axesProp.TickLabelInterpreter, ...
            'BackgroundColor', 'w');
        firstObsID = ...
            config.passiveActor{passiveActorToPlot(1)}.actorID;
        tArray = config.actor{firstObsID}.startTime + ...
            config.actor{firstObsID}.actionInterval*(observationToPlot-1);
        if nargin == 0
            numPt = 4;
        else
            numPt = numPtUser;
        end
    end

    function accordAddObservationCount(passiveActorInd, molInd, customTxt)
        numCounter = numCounter + 1;
        hObsCount(numCounter) = annotation(hFig(1), ...
            'textbox', [0.01 0.01 0.1 0.1], ...
            'Tag', ['Counter ' num2str(numCounter)], ...
            'String', [customTxt ' VALUE'], ...
            'FontName', axesProp.FontName, ...
            'FontSize', axesProp.FontSize, ...
            'FontWeight', axesProp.FontWeight, ...            
            'Interpreter', axesProp.TickLabelInterpreter, ...
            'BackgroundColor', 'w');
        counterArray{numCounter} = ...
            squeeze(data.passiveRecordCount{passiveActorInd}(curRepeat,molInd,observationToPlot(1:numObs)));
        counterText{numCounter} = customTxt;
    end

    % Add anchor camera point to apply at specified frames
    function accordAddCameraAnchor(curCameraFrames)
        bDynamicCamera = true;
        numCameraAnchor = numCameraAnchor + 1;
        cameraAnchorArray{1,numCameraAnchor} = get(hAxes(1), ...
            {'CameraPosition','CameraTarget', ...
            'CameraViewAngle','CameraUpVector'});
        frameCameraAnchor(curCameraFrames) = numCameraAnchor;
    end

    % View existing camera anchor
    function accordViewCameraAnchor(curAnchor)
        if bDynamicCamera && curAnchor <= numCameraAnchor
            set(hAxes(1), {'CameraPosition','CameraTarget',...
                'CameraViewAngle','CameraUpVector'}, ...
                cameraAnchorArray{curAnchor});
        else
            warning('Cannot view camera angle %d. %d anchors have been defined',...
                curAnchor, numCameraAnchor);
        end
    end

    function accordAddAnnotation(curAnnotFrames)
        bDynamicAnnotation = true;
        numAnnotatedFrames = numAnnotatedFrames + 1;
        hAnnotAxes = findall(hFig(1), 'Tag', 'scribeOverlay');
        hAnnot = get(hAnnotAxes, 'Children');
        if bDispTime
            if iscell(hAnnot)
                hAnnot = [hAnnot{:}];
            end
            % Find and exclude time display
            for hCur = 1:length(hAnnot)
                if strcmp('Timer Box', get(hAnnot(hCur),'Tag'))
                    hAnnot(hCur) = [];
                    break
                end
            end
        end
        set(hAnnot, 'Tag', num2str(numAnnotatedFrames));
        copyobj(hAnnot, hAnnotInvisAxes);
        frameAnnotation(curAnnotFrames) = numAnnotatedFrames;
    end
end