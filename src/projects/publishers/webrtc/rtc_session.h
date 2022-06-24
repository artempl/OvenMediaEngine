//==============================================================================
//
//  OvenMediaEngine
//
//  Created by Getroot
//  Copyright (c) 2018 AirenSoft. All rights reserved.
//
//==============================================================================
#pragma once
#include <unordered_set>
#include <monitoring/monitoring.h>
#include <modules/http/server/web_socket/web_socket_session.h>

#include "base/info/media_track.h"
#include "base/publisher/session.h"
#include "modules/sdp/session_description.h"
#include "modules/ice/ice_port.h"
#include "modules/rtp_rtcp/rtp_rtcp.h"
#include "modules/rtp_rtcp/rtp_packetizer_interface.h"
#include "modules/dtls_srtp/dtls_transport.h"

#include "rtc_playlist.h"

/*	Node Connection
 * [  RTP_RTCP ]
 * [SRTP] [SCTP]				
 * [    DTLS   ]
 * [  ICE/STUN ]
 * [ RtcSession](Edge Node)	 ----> Send packet
 *		  <----------------------- Recv packet
 *   
 */

class WebRtcPublisher;
class RtcApplication;
class RtcStream;

class RtcSession : public pub::Session, public RtpRtcpInterface, public ov::Node
{
public:
	static std::shared_ptr<RtcSession> Create(const std::shared_ptr<WebRtcPublisher> &publisher,
											  const std::shared_ptr<pub::Application> &application,
	                                          const std::shared_ptr<pub::Stream> &stream,
											  const ov::String &file_name,
	                                          const std::shared_ptr<const SessionDescription> &offer_sdp,
	                                          const std::shared_ptr<const SessionDescription> &peer_sdp,
	                                          const std::shared_ptr<IcePort> &ice_port,
											  const std::shared_ptr<http::svr::ws::WebSocketSession> &ws_session);

	RtcSession(const info::Session &session_info,
			const std::shared_ptr<WebRtcPublisher> &publisher,
			const std::shared_ptr<pub::Application> &application,
	        const std::shared_ptr<pub::Stream> &stream,
			const ov::String &file_name,
	        const std::shared_ptr<const SessionDescription> &offer_sdp,
	        const std::shared_ptr<const SessionDescription> &peer_sdp,
	        const std::shared_ptr<IcePort> &ice_port,
			const std::shared_ptr<http::svr::ws::WebSocketSession> &ws_session);
	~RtcSession() override;

	bool Start() override;
	bool Stop() override;

	bool ChangeRendition(const ov::String &rendition_name);

	void SetSessionExpiredTime(uint64_t expired_time);

	const std::shared_ptr<const SessionDescription>& GetPeerSDP() const;
	const std::shared_ptr<const SessionDescription>& GetOfferSDP() const;
	const std::shared_ptr<http::svr::ws::WebSocketSession>& GetWSClient();

	// pub::Session Interface
	void SendOutgoingData(const std::any &packet) override;
	void OnMessageReceived(const std::any &message) override;
	
	// RtpRtcp Interface
	void OnRtpFrameReceived(const std::vector<std::shared_ptr<RtpPacket>> &rtp_packets) override;
	void OnRtcpReceived(const std::shared_ptr<RtcpInfo> &rtcp_info) override;

	// ov::Node Interface
	bool OnDataReceivedFromPrevNode(NodeType from_node, const std::shared_ptr<ov::Data> &data) override;
	bool OnDataReceivedFromNextNode(NodeType from_node, const std::shared_ptr<const ov::Data> &data) override;

private:
	bool ProcessNACK(const std::shared_ptr<RtcpInfo> &rtcp_info);

	bool IsSelectedPacket(const std::shared_ptr<const RtpPacket> &rtp_packet);

	std::shared_ptr<WebRtcPublisher>	_publisher;

	std::shared_ptr<RtpRtcp>            _rtp_rtcp;
	std::shared_ptr<SrtpTransport>      _srtp_transport;
	std::shared_ptr<DtlsTransport>      _dtls_transport;

	std::shared_ptr<const SessionDescription> _offer_sdp;
	std::shared_ptr<const SessionDescription> _peer_sdp;
	std::shared_ptr<IcePort>            _ice_port;
	std::shared_ptr<http::svr::ws::WebSocketSession> 	_ws_session; // Signalling  

	ov::String 							_file_name;
	std::shared_ptr<const RtcPlaylist>	_playlist;
	std::shared_ptr<const RtcRendition>	_current_rendition = nullptr;
	std::shared_ptr<const RtcRendition>	_next_rendition = nullptr;

	uint8_t                             _video_payload_type = 0;
	uint32_t							_video_ssrc = 0;
	
	uint8_t                             _audio_payload_type = 0;
	uint32_t							_audio_ssrc = 0;

	bool								_red_enabled = false;
	bool								_rtx_enabled = false;

	uint16_t							_rtx_sequence_number = 1;
	uint64_t							_session_expired_time = 0;

	std::shared_mutex					_start_stop_lock;

	uint16_t _video_rtp_sequence_number = 0;
	uint16_t _audio_rtp_sequence_number = 0;

	ov::StopWatch _abr_test_watch;
	bool _changed = false;
};